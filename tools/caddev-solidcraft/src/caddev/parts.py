"""Reusable, lazily built CAD parts with overrideable design properties."""
from abc import ABC, abstractmethod
from functools import cached_property

from build123d import Shape
from .geometry import ComponentGeometry


class CadPart(ABC):
    """Implement build(); override dimension properties for nominal design sizes.

    Geometry is built once per object. Treat construction parameters as immutable:
    construct a new instance when parameters change. Shapes returned by .shape
    are independent copies, so placing one instance cannot move another.
    """

    @abstractmethod
    def build(self) -> Shape | ComponentGeometry:
        """Construct geometry, named bodies, and local datum frames."""

    @cached_property
    def geometry(self) -> Shape | ComponentGeometry:
        result = self.build()
        if not isinstance(result, (Shape, ComponentGeometry)):
            raise TypeError('CadPart.build() must return Shape or ComponentGeometry')
        return result

    @cached_property
    def _built(self):
        from .exporter import build_component
        return build_component(self.geometry, type(self).__name__)

    @property
    def shape(self) -> Shape:
        from copy import deepcopy
        return deepcopy(self._built.shape)

    @property
    def bounds(self):
        """Geometric bounding box in the part's local coordinate frame."""
        return self._built.shape.bounding_box()

    @property
    def width(self) -> float:
        """X extent; subclasses may override for a nominal dimension."""
        return self.bounds.size.X

    @property
    def depth(self) -> float:
        """Y extent; subclasses may override for a nominal dimension."""
        return self.bounds.size.Y

    @property
    def height(self) -> float:
        """Z extent; subclasses may override for a nominal stacking dimension."""
        return self.bounds.size.Z

    @property
    def datums(self):
        from copy import deepcopy
        return deepcopy(self._built.datums)
