from pathlib import Path
from caddev.builder import BuildSession
from caddev.log import error, success


def print_report(report):
    for id in report.built:
        success(f'Exported {id}')
    for id, message in report.errors.items():
        error(f'{id}: {message}')


def build(root: Path) -> None:
    report = BuildSession(root).rebuild()
    print_report(report)
    if report.errors:
        raise SystemExit(1)
