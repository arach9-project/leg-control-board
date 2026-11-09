"""Load project sources without timestamp-based bytecode caching."""
from __future__ import annotations
import importlib
import importlib.abc
import importlib.machinery
import importlib.util
import sys
from contextlib import contextmanager
from pathlib import Path
from .dependencies import IGNORED
from .project import Project


def is_local(path: Path, root: Path) -> bool:
    try:
        parts = path.resolve().relative_to(root).parts
        return not any(p in IGNORED for p in parts)
    except (ValueError, OSError):
        return False


class FreshSourceLoader(importlib.machinery.SourceFileLoader):
    def get_code(self, fullname):
        return self.source_to_code(self.get_data(self.path), self.path)


class FreshFinder(importlib.abc.MetaPathFinder):
    def __init__(self, root):
        self.root = root

    def find_spec(self, fullname, path=None, target=None):
        spec = importlib.machinery.PathFinder.find_spec(fullname, path)
        if spec and spec.origin and spec.origin.endswith('.py') and is_local(Path(spec.origin), self.root):
            spec.loader = FreshSourceLoader(fullname, spec.origin)
            return spec
        return None


@contextmanager
def fresh_sources(root):
    finder = FreshFinder(root.resolve())
    sys.meta_path.insert(0, finder)
    try:
        yield
    finally:
        sys.meta_path.remove(finder)


def unload_project_modules(root):
    for name, module in list(sys.modules.items()):
        path = getattr(module, '__file__', None)
        local = (path and is_local(Path(path), root.resolve())) or any(
            is_local(Path(p), root.resolve()) for p in getattr(module, '__path__', ())
        )
        if local and name != 'caddev' and not name.startswith('caddev.'):
            sys.modules.pop(name, None)
    sys.modules.pop('caddev_user_project', None)
    importlib.invalidate_caches()


def load_project(root: Path) -> Project:
    root = root.resolve()
    path = root / 'project.py'
    if not path.exists():
        raise RuntimeError(f'no project.py found in {root}')
    if str(root) not in sys.path:
        sys.path.insert(0, str(root))
    loader = FreshSourceLoader('caddev_user_project', str(path))
    spec = importlib.util.spec_from_loader(loader.name, loader)
    module = importlib.util.module_from_spec(spec)
    sys.modules[loader.name] = module
    with fresh_sources(root):
        loader.exec_module(module)
    project = getattr(module, 'project', None)
    if not isinstance(project, Project):
        raise TypeError('project.py must define project = Project(...)')
    project.validate()
    return project


def reload_project(root):
    unload_project_modules(root)
    return load_project(root)
