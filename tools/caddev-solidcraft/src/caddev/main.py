"""caddev command-line interface."""
import argparse
from pathlib import Path
from .commands.add import add_part
from .commands.build import build
from .commands.dev import dev
from .commands.init import init_project
from .commands.status import status
from .commands.assembly import assembly


def create_parser():
    parser = argparse.ArgumentParser(prog='caddev', description='Watch build123d projects and export STEP files.')
    commands = parser.add_subparsers(dest='command')
    dev_parser = commands.add_parser('dev', help='Build STEP files and watch Python sources.')
    dev_parser.add_argument('--preview', nargs='?', const='auto', metavar='TARGET',
                            help='Launch a disposable SolidCraft live preview (default: first assembly or part).')
    dev_parser.add_argument('--solidcraft', metavar='EXECUTABLE', help='SolidCraft executable or AppImage path.')
    persistent = commands.add_parser('assembly', help='Open a persistent GUI-owned assembly and watch part exports.')
    persistent.add_argument('--document', default='assembly/gearbox.solidcraft')
    persistent.add_argument('--seed', help='One-time layout migration, module:symbol; never overwrites a document')
    persistent.add_argument('--ground', default='housing')
    persistent.add_argument('--seed-only', action='store_true')
    persistent.add_argument('--solidcraft', help='SolidCraft executable or AppImage')
    persistent.add_argument('--insert', help='Explicitly insert a registered part into the open assembly')
    persistent.add_argument('--name', help='Name for the explicitly inserted component')
    commands.add_parser('build', help='Export parts and assemblies once.')
    commands.add_parser('status', help='List project targets and local exports.')
    create = commands.add_parser('create', aliases=['init'], help='Create a CAD project with a justfile.')
    create.add_argument('name')
    add = commands.add_parser('add', help='Add a project item.')
    kinds = add.add_subparsers(dest='add_type', required=True)
    part = kinds.add_parser('part')
    part.add_argument('name')
    return parser


def main():
    args = create_parser().parse_args()
    root = Path.cwd()
    try:
        match args.command or 'dev':
            case 'dev': dev(root, preview=getattr(args, 'preview', None), executable=getattr(args, 'solidcraft', None))
            case 'assembly': assembly(root, args.document, args.seed, args.ground, args.seed_only, args.solidcraft, args.insert, args.name)
            case 'build': build(root)
            case 'status': status(root)
            case 'create' | 'init': init_project(Path(args.name), project_name=Path(args.name).resolve().name)
            case 'add': add_part(root, name=args.name)
    except Exception as exc:
        print(f'caddev: {exc}')
        raise SystemExit(1) from exc


if __name__ == '__main__':
    main()
