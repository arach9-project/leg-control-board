"""Convert factory results to non-destructive STEP assembly trees."""
from __future__ import annotations
from copy import deepcopy
from dataclasses import dataclass
from pathlib import Path
import os
import tempfile

from build123d import Color, Compound, Location, Shape, Solid, Part, Unit
from OCP.BRepBuilderAPI import BRepBuilderAPI_Transform
from OCP.TopLoc import TopLoc_Location
from .step import export_step
from .geometry import ComponentGeometry
from .project import Assembly


@dataclass
class BuiltTarget:
    shape: Shape
    datums: dict[str, Location]


def build_component(result, name: str) -> BuiltTarget:
    from .parts import CadPart
    if isinstance(result, CadPart):
        result = result.geometry
    if isinstance(result, Shape):
        result = ComponentGeometry.from_shape(result, name=result.label or name)
    if not isinstance(result, ComponentGeometry):
        raise TypeError('Factory must return a CadPart, build123d Shape, or ComponentGeometry')
    bodies = []
    for key, feature in result.features.items():
        source = feature.shape.shape if isinstance(feature.shape, CadPart) else feature.shape
        if not isinstance(source, Shape) or source.wrapped is None:
            raise TypeError(f'{name}/{key}: feature must contain a shape')
        solids = source.solids()
        if not solids:
            raise ValueError(f'{name}/{key}: feature must contain at least one solid body')
        for index, solid in enumerate(solids):
            placed = deepcopy(solid).moved(feature.location)
            # Apply the complete rigid transform to the actual geometry, exactly
            # once. A body is a subshape, not a separately placed STEP product.
            raw = placed.wrapped
            baked = BRepBuilderAPI_Transform(
                raw.Located(TopLoc_Location()), raw.Location().Transformation(), True
            ).Shape()
            body = Solid(baked)
            body.label = feature.name or key
            if len(solids) > 1:
                body.label += f' ({index + 1})'
            body.color = source.color
            if feature.appearance and feature.appearance.color is not None:
                body.color = Color(*feature.appearance.color)
            bodies.append(body)
    if not bodies:
        raise ValueError(f'{name}: component is empty')
    shape = Part(Compound(bodies).wrapped, label=name)
    shape._caddev_bodies = bodies
    return BuiltTarget(shape, deepcopy(result.datums))


def build_assembly(assembly: Assembly, built: dict[str, BuiltTarget]) -> BuiltTarget:
    instances = {i.id: i for i in assembly.instances}
    locations: dict[str, Location] = {}
    def locate(id: str) -> Location:
        if id in locations:
            return locations[id]
        instance = instances[id]
        mate = instance.mate
        if mate:
            target = instances[mate.target_instance]
            try:
                source_datum = built[instance.component].datums[mate.source_datum]
                target_datum = built[target.component].datums[mate.target_datum]
            except KeyError as exc:
                raise ValueError(f'{assembly.id}/{id}: missing datum {exc}') from exc
            location = locate(target.id) * target_datum * mate.offset * source_datum.inverse()
        else:
            location = instance.location or Location()
        locations[id] = location
        return location
    children = []
    for instance in assembly.instances:
        shape = deepcopy(built[instance.component].shape).moved(locate(instance.id))
        shape.label = instance.name or instance.id
        shape._caddev_instance_id = instance.id
        shape._caddev_component_id = instance.component
        children.append(shape)
    return BuiltTarget(Compound(label=assembly.name or assembly.id, children=children),
                       deepcopy(assembly.datums))


def write_step(target: BuiltTarget, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary = tempfile.mkstemp(prefix=f'.{path.stem}-', suffix='.step', dir=path.parent)
    os.close(fd)
    try:
        if not export_step(target.shape, temporary, unit=Unit.MM):
            raise RuntimeError(f'STEP export failed: {path}')
        os.replace(temporary, path)
    finally:
        Path(temporary).unlink(missing_ok=True)
