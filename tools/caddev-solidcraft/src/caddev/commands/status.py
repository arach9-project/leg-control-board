from pathlib import Path
from caddev.loader import load_project


def status(root: Path) -> None:
    project = load_project(root)
    print(f'Project: {root}')
    for kind, targets in [('parts', project.components), ('assemblies', project.assemblies)]:
        for target in targets:
            path = root / 'build' / kind / f'{target.id}.step'
            state = f'present ({path.stat().st_size} bytes)' if path.exists() else 'missing'
            print(f'{kind}/{target.id}: {state} — {path}')
    print('Present files are last successful exports; status does not rebuild or verify freshness.')
