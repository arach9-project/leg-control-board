"""Conservative static dependency tracking for project-local Python modules."""
import ast
import os
from pathlib import Path

IGNORED = {'.venv', 'venv', '.git', '.cache', '__pycache__', 'build', 'dist', '.codex', '.agents'}


class Dependencies:
    def __init__(self, root: Path):
        self.root = root
        self.files = set()
        for directory, dirs, files in os.walk(root):
            dirs[:] = [d for d in dirs if d not in IGNORED]
            self.files.update((Path(directory) / f).resolve() for f in files if f.endswith('.py'))
        self.modules = {}
        for path in self.files:
            parts = list(path.relative_to(root).with_suffix('').parts)
            if parts[-1] == '__init__':
                parts.pop()
            self.modules['.'.join(parts)] = path
        self.edges = {p: set() for p in self.files}
        self.uncertain = False
        for module, path in self.modules.items():
            try:
                tree = ast.parse(path.read_text())
            except (SyntaxError, OSError):
                self.uncertain = True
                continue
            for node in ast.walk(tree):
                names = []
                if isinstance(node, ast.Import):
                    names = [alias.name for alias in node.names]
                    if any(name in {'importlib', 'runpy'} for name in names):
                        self.uncertain = True
                elif isinstance(node, ast.ImportFrom):
                    base = node.module or ''
                    if base in {'importlib', 'runpy'}:
                        self.uncertain = True
                    if node.level:
                        package = module.split('.') if path.name == '__init__.py' else module.split('.')[:-1]
                        package = package[:len(package) - node.level + 1]
                        base = '.'.join(package + ([base] if base else []))
                    names = [base] + [f'{base}.{alias.name}' for alias in node.names]
                elif isinstance(node, ast.Call):
                    f = node.func
                    if (isinstance(f, ast.Name) and f.id in {'__import__', 'import_module', 'eval', 'exec'}) or (
                        isinstance(f, ast.Attribute) and f.attr in {'import_module', 'exec_module', 'spec_from_file_location'}):
                        self.uncertain = True
                for name in names:
                    chunks = name.split('.')
                    for n in range(1, len(chunks) + 1):
                        dependency = self.modules.get('.'.join(chunks[:n]))
                        if dependency:
                            self.edges[path].add(dependency)

    def closure(self, paths):
        result = set(paths)
        pending = list(paths)
        while pending:
            for child in self.edges.get(pending.pop(), ()):
                if child not in result:
                    result.add(child)
                    pending.append(child)
        return result

    def module_paths(self, module):
        chunks = module.split('.')
        return self.closure({self.modules['.'.join(chunks[:n])]
                             for n in range(1, len(chunks)+1)
                             if '.'.join(chunks[:n]) in self.modules})
