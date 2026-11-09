"""Add components to a caddev project."""

from __future__ import annotations

import re
import ast
from pathlib import Path

from caddev.log import success


PART_TEMPLATE = '''\
from caddev import CadPart


class Model(CadPart):
    def build(self):
        raise NotImplementedError


part = Model()
'''


def add_part(
    root: Path,
    *,
    name: str,
) -> None:
    root = root.resolve()

    project_file = root / "project.py"
    parts_dir = root / "parts"

    if not project_file.exists():
        raise RuntimeError(
            f"no caddev project found in {root}"
        )

    module_name = _snake_case(name)
    if not module_name.isidentifier():
        raise ValueError("Part name must produce a valid Python identifier")
    display_name = _display_name(module_name)

    part_file = parts_dir / f"{module_name}.py"

    if part_file.exists():
        raise RuntimeError(
            f"part already exists: {part_file}"
        )

    parts_dir.mkdir(
        parents=True,
        exist_ok=True,
    )

    original = project_file.read_text(encoding="utf-8")
    _append_component(project_file, component_id=module_name,
                      name=display_name, module=f"parts.{module_name}")
    try:
        part_file.write_text(PART_TEMPLATE, encoding="utf-8")
    except Exception:
        project_file.write_text(original, encoding="utf-8")
        raise

    success(f"Created part: {module_name}")


def _append_component(
    project_file: Path,
    *,
    component_id: str,
    name: str,
    module: str,
) -> None:
    source = project_file.read_text(
        encoding="utf-8"
    )

    tree = ast.parse(source)
    calls = [node for node in ast.walk(tree) if isinstance(node, ast.Call)
             and isinstance(node.func, ast.Name) and node.func.id == "Project"]
    lists = [kw.value for call in calls for kw in call.keywords
             if kw.arg == "components" and isinstance(kw.value, ast.List)]
    if len(lists) != 1:
        raise RuntimeError("project.py must contain one literal Project(components=[...]) list")
    node = lists[0]
    lines = source.splitlines(keepends=True)
    # AST columns are UTF-8 byte offsets.
    encoded = source.encode("utf-8")
    offset = sum(len(line.encode("utf-8")) for line in lines[:node.end_lineno - 1]) + node.end_col_offset - 1
    prefix = encoded[:offset].decode("utf-8")
    suffix = encoded[offset:].decode("utf-8")
    # Insert after the last element, before any trailing comments/whitespace.
    if node.elts:
        last = node.elts[-1]
        end = sum(len(line.encode("utf-8")) for line in lines[:last.end_lineno - 1]) + last.end_col_offset
        tail = encoded[end:offset].decode("utf-8")
        if not tail.lstrip().startswith(","):
            prefix = encoded[:end].decode("utf-8") + "," + tail
    entry = f"\n        Component(id={component_id!r}, name={name!r}, module={module!r}, factory='part'),\n    "
    result = prefix + entry + suffix
    ast.parse(result)
    project_file.write_text(result, encoding="utf-8")


def _snake_case(value: str) -> str:
    value = re.sub(
        r"(?<!^)(?=[A-Z])",
        "_",
        value,
    )
    value = re.sub(
        r"[^A-Za-z0-9]+",
        "_",
        value,
    )

    return value.strip("_").lower()


def _display_name(value: str) -> str:
    return value.replace("_", " ").title()
