from pathlib import Path
from caddev.builder import BuildSession
from caddev.commands.build import print_report
from caddev.log import info, warning
from caddev.preview import SolidCraftPreview, preview_path
from caddev.preview_scene import preview_scene
from caddev.watcher import watch_project


def dev(root: Path, preview: str | None = None, executable: str | None = None) -> None:
    session = BuildSession(root)
    viewer = None
    try:
        if preview is not None:
            viewer = SolidCraftPreview(root, executable)
            info('SolidCraft is a disposable preview; Python controls the design.')
        def rebuild(paths=None):
            report = session.rebuild(paths)
            print_report(report)
            if viewer and session.project and 'project' not in report.errors:
                try:
                    path = preview_path(session.project, root, preview)
                    if path.stem in report.built:
                        viewer.submit(path, preview_scene(session.cache[path.stem].shape, root, path.stem))
                except (ValueError, OSError) as exc:
                    warning(str(exc))
        rebuild()
        info('Watching Python sources; press Ctrl+C to stop')
        watch_project(root, rebuild)
    except KeyboardInterrupt:
        info('Watcher stopped')
    finally:
        if viewer:
            viewer.close()
