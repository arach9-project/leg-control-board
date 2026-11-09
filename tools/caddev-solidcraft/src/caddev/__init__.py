# Use the bundled font search configuration unless the caller supplies one.
import os
from pathlib import Path
os.environ.setdefault('FONTCONFIG_FILE', str(Path(__file__).parent / 'fontconfig' / 'fonts.conf'))

from .geometry import Appearance, ComponentGeometry, FeatureGeometry, Material
from .parts import CadPart
from .project import Assembly, Component, Instance, Mate, Project

__all__ = ['Appearance', 'Assembly', 'CadPart', 'Component', 'ComponentGeometry',
           'FeatureGeometry', 'Instance', 'Material', 'Mate', 'Project']
