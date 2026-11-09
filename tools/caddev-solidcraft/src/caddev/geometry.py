from __future__ import annotations

from dataclasses import dataclass, field
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from .parts import CadPart

from build123d import Location, Part, Shape


Color = tuple[float, float, float]


@dataclass(frozen=True)
class Appearance:
    """Visual appearance of a CAD feature."""

    color: Color | None = None
    transparency: int | None = None
    shininess: float | None = None

    def __post_init__(self) -> None:
        if self.color is not None:
            if len(self.color) != 3:
                raise ValueError(
                    "color must contain exactly 3 values"
                )

            if not all(
                0.0 <= value <= 1.0
                for value in self.color
            ):
                raise ValueError(
                    "color values must be between 0 and 1"
                )

        if (
            self.transparency is not None
            and not 0 <= self.transparency <= 100
        ):
            raise ValueError(
                "transparency must be between 0 and 100"
            )

        if (
            self.shininess is not None
            and not 0.0 <= self.shininess <= 1.0
        ):
            raise ValueError(
                "shininess must be between 0 and 1"
            )


@dataclass(frozen=True)
class Material:
    """Engineering material metadata."""

    name: str

    density: float | None = None
    youngs_modulus: float | None = None
    poisson_ratio: float | None = None

    description: str | None = None


@dataclass
class FeatureGeometry:
    """A named body (or disconnected bodies) within one physical part."""

    shape: Shape | CadPart
    name: str | None = None
    location: Location = field(
        default_factory=Location
    )
    appearance: Appearance | None = None
    material: Material | None = None


@dataclass
class ComponentGeometry:
    """Geometry and interfaces belonging to one physical component."""

    features: dict[str, FeatureGeometry] = field(
        default_factory=dict
    )

    datums: dict[str, Location] = field(
        default_factory=dict
    )

    @classmethod
    def from_shape(
        cls,
        shape: Part,
        *,
        name: str = "Geometry",
        appearance: Appearance | None = None,
        material: Material | None = None,
        datums: dict[str, Location] | None = None,
    ) -> ComponentGeometry:
        """Convenience constructor for a single-feature component."""

        return cls(
            features={
                "geometry": FeatureGeometry(
                    shape=shape,
                    name=name,
                    appearance=appearance,
                    material=material,
                )
            },
            datums=datums or {},
        )
