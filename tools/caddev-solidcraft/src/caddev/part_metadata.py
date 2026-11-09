"""Part-local interchange metadata, published after a successful STEP export."""

import hashlib
import json
import os
import tempfile
from pathlib import Path


def matrix(location):
    transform = location.wrapped.Transformation()
    return [[transform.Value(r, c) for c in range(1, 5)] for r in range(1, 4)] + [[0, 0, 0, 1]]


def geometry_hash(step):
    _, marker, data = step.partition("DATA;")
    if not marker:
        raise ValueError("STEP export has no DATA section")
    return hashlib.sha256(data.encode()).hexdigest()


def atomic_json(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, temp = tempfile.mkstemp(dir=path.parent, prefix="." + path.name)
    try:
        with os.fdopen(fd, "w") as stream:
            json.dump(value, stream, indent=2, allow_nan=False)
            stream.write("\n")
        os.replace(temp, path)
    finally:
        Path(temp).unlink(missing_ok=True)


def write_metadata(target, part_id, path):
    atomic_json(
        path.with_suffix(".json"),
        {
            "schema": 1,
            "part_id": part_id,
            "units": "mm",
            "up_axis": "Z",
            "matrix_convention": "row-major; column vectors; part-local to parent",
            "geometry_hash": geometry_hash(path.read_text()),
            "datums": {name: matrix(loc) for name, loc in target.datums.items()},
        },
    )


def read_part(root, part_id):
    path = Path(root) / "build" / "parts" / f"{part_id}.step"
    metadata = json.loads(path.with_suffix(".json").read_text())
    step = path.read_text()
    if (
        metadata["schema"] != 1
        or metadata["part_id"] != part_id
        or metadata["units"] != "mm"
        or metadata["geometry_hash"] != geometry_hash(step)
    ):
        raise ValueError(f"{part_id}: STEP and metadata are inconsistent; rebuild the part")
    return metadata, step
