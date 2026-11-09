"""Watch project sources, excluding generated outputs and environments."""
from pathlib import Path
from watchfiles import watch
from .dependencies import IGNORED


def source_filter(root: Path, path: str) -> bool:
    path = Path(path).resolve()
    try:
        relative = path.relative_to(root.resolve())
    except ValueError:
        return False
    return path.suffix == '.py' and not any(p in IGNORED for p in relative.parts)


def watch_project(root, on_change):
    root = root.resolve()
    for changes in watch(root, debounce=300,
                         watch_filter=lambda change, path: source_filter(root, path)):
        paths = {Path(path).resolve() for _, path in changes}
        if paths:
            on_change(paths)
