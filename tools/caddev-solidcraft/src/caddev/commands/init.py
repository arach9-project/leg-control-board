"""Create a new caddev project."""

from __future__ import annotations

from pathlib import Path
import json
import re


PROJECT_TEMPLATE = '''\
from caddev import Component, Project
from assemblies.pair import pair


project = Project(
    components=[
        Component(
            id="example",
            name="Example Part",
            module="parts.example",
            factory="part",
        ),
    ],
    assemblies=[pair],
)
'''


EXAMPLE_PART_TEMPLATE = '''\
from build123d import Align, Box
from caddev import CadPart


class Example(CadPart):
    @property
    def height(self) -> float:
        return 10.0

    def build(self):
        return Box(20, 20, self.height, align=(Align.CENTER, Align.CENTER, Align.MIN))


part = Example()
'''



ASSEMBLY_TEMPLATE = '''\
from build123d import Location
from caddev import Assembly, Instance
from parts.example import part


pair = Assembly(
    id="pair",
    instances=[
        Instance("first", "example"),
        Instance("second", "example", location=Location((0, 0, part.height))),
    ],
)
'''


JUSTFILE_TEMPLATE = '''\
# Start a disposable SolidCraft live preview (or choose a part/assembly ID).
run target="auto":
    uv run caddev dev --preview {{quote(target)}}
'''


README_TEMPLATE = '''\
# CAD project

Requires Python 3.11+, uv, and just.

```sh
just run
```

This installs dependencies through uv, exports STEP files, watches Python sources,
and opens a dedicated SolidCraft live preview of the pair assembly. Use
`just run example` to preview the part. Press Ctrl+C to stop and close the preview. For a one-shot export, run
`uv run caddev build`.

Parts live in `parts/`, assembly definitions in `assemblies/`, shared geometry in
`geometry/`, and helpers in `utils/`. Register parts and assemblies in `project.py`.
The example assembly stacks two instances using `part.height`. Part modules expose
a `CadPart` subclass and a shared `part` object. Override `height`, `width`, or
`depth` properties for nominal dimensions; defaults come from geometry bounds.
Features of a `ComponentGeometry` export as named bodies inside one part.

The managed preview refreshes after successful builds. Python controls this window;
edits made inside it are replaced on refresh. Normal editing windows are untouched.
SolidCraft must be installed (on PATH, via a Linux desktop entry, or specified with
SOLIDCRAFT_EXECUTABLE). `uv run caddev dev` watches and exports without a viewer.

Generated STEP files are under `build/parts/` and `build/assemblies/`, ignored by Git. `uv run caddev status` lists the last successful exports.

When generated from a caddev source checkout, `pyproject.toml` points to that
checkout as an editable dependency. Update that path if you move the checkout
or share this project with another machine.
'''


PYPROJECT_TEMPLATE = '''\
[project]
name = "{project_name}"
version = "0.1.0"
requires-python = ">=3.11"

dependencies = [
    "build123d",
    "caddev",
]

[tool.basedpyright]
typeCheckingMode = "standard"

'''


GITIGNORE_TEMPLATE = '''\
.venv/
__pycache__/
*.pyc
dist/
build/
'''


PACKAGE_DIRS = (
    "geometry",
    "parts",
    "assemblies",
    "utils",
)


def init_project(
    root: Path,
    *,
    project_name: str,
) -> None:
    root = root.resolve()

    project_name = re.sub(r"[^A-Za-z0-9._-]+", "-", project_name).strip("._-")
    if not project_name:
        raise ValueError("Project name must contain a letter or digit")

    if root.exists() and any(root.iterdir()):
        raise RuntimeError(
            f"directory is not empty: {root}"
        )

    root.mkdir(
        parents=True,
        exist_ok=True,
    )

    for directory in PACKAGE_DIRS:
        package = root / directory

        package.mkdir(
            parents=True,
            exist_ok=True,
        )

        (package / "__init__.py").write_text(
            "",
            encoding="utf-8",
        )

    (root / "project.py").write_text(
        PROJECT_TEMPLATE,
        encoding="utf-8",
    )

    configuration = PYPROJECT_TEMPLATE.format(project_name=project_name)
    checkout = Path(__file__).resolve().parents[3]
    if (checkout / "src" / "caddev" / "commands" / "init.py").resolve() == Path(__file__).resolve() and (checkout / "pyproject.toml").is_file():
        configuration += (
            "[tool.uv.sources]\n"
            f"caddev = {{ path = {json.dumps(str(checkout))}, editable = true }}\n"
        )
    (root / "pyproject.toml").write_text(configuration, encoding="utf-8")

    for name, content in {
        "justfile": JUSTFILE_TEMPLATE,
        "README.md": README_TEMPLATE,
        "assemblies/pair.py": ASSEMBLY_TEMPLATE,
    }.items():
        (root / name).write_text(content, encoding="utf-8")

    (root / ".gitignore").write_text(
        GITIGNORE_TEMPLATE,
        encoding="utf-8",
    )

    (root / "parts" / "example.py").write_text(
        EXAMPLE_PART_TEMPLATE,
        encoding="utf-8",
    )

    print(f"Created CAD project: {root}")
