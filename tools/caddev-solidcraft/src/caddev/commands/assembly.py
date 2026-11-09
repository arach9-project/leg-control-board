"""Run a persistent, user-edited SolidCraft assembly with part-only builds."""

import fcntl
import importlib
import sys
from pathlib import Path

from watchfiles import watch
from caddev.builder import BuildSession
from caddev.commands.build import print_report
from caddev.log import info, warning
from caddev.persistent import PersistentApp, Deferred, KEY
from caddev.watcher import source_filter


def assembly(
    root,
    document,
    seed=None,
    ground="housing",
    seed_only=False,
    executable=None,
    insert=None,
    name=None,
):
    root = Path(root).resolve()
    document = (root / document).resolve()
    (root / "build").mkdir(exist_ok=True)
    with (root / "build/.assembly-watcher.lock").open("w") as lock:
        try:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            raise RuntimeError("A persistent assembly watcher is already running for this project")
        if seed and document.exists():
            raise FileExistsError(f"Refusing to overwrite {document}")
        if not seed and not document.exists():
            raise FileNotFoundError(f"{document} is missing; run just seed once")
        session = BuildSession(root)
        report = session.rebuild()
        print_report(report)
        if not session.project or session.project.assemblies:
            raise ValueError("Persistent mode requires a parts-only Project (assemblies=[])")
        app = PersistentApp(root, document, executable)
        try:
            app.start()
            if seed:
                if report.errors:
                    raise RuntimeError("Fix part build errors before seeding the assembly")
                sys.path.insert(0, str(root))
                module, symbol = seed.split(":", 1)
                source = getattr(importlib.import_module(module), symbol)
                app.seed(source, session.cache, ground)
                info(f"Created {document}; save future assembly edits in SolidCraft")
            if insert:
                doc = app.ready()
                if insert not in session.cache:
                    raise ValueError(f"Part {insert} is not available")
                from caddev.preview_scene import IDENTITY

                app.insert(insert, name or insert, IDENTITY, doc["metadata"][KEY]["id"])
                info(f"Inserted {insert}; place and constrain it in SolidCraft, then Save")
            if seed_only or insert:
                return
            pending = True
            last_message = None

            def refresh():
                nonlocal pending, last_message
                try:
                    count, diagnostics = app.refresh(set(session.cache))
                    if count:
                        info(
                            f"Updated {count} part imports; assembly edits remain in SolidCraft (Save to keep them)"
                        )
                    for message in diagnostics:
                        warning(message)
                    pending = False
                    last_message = None
                except Deferred as exc:
                    if str(exc) != last_message:
                        info(str(exc))
                        last_message = str(exc)
                except ValueError as exc:
                    warning(str(exc))
                    pending = False

            refresh()
            info("Watching parts; Ctrl+C stops synchronization and leaves SolidCraft open")
            for changes in watch(
                root,
                debounce=300,
                rust_timeout=500,
                yield_on_timeout=True,
                watch_filter=lambda change, path: source_filter(root, path),
            ):
                if changes:
                    report = session.rebuild({Path(p).resolve() for _, p in changes})
                    print_report(report)
                    if "project" not in report.errors:
                        pending = True
                if pending:
                    refresh()
        except KeyboardInterrupt:
            info("Watcher stopped. SolidCraft and unsaved assembly edits remain open.")
        finally:
            app.close()
