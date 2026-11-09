
"""Runtime checks for the external CAD development process.

caddev runs inside the user's normal Python environment. It does not download
Python, uv, build123d, CadQuery, or any other runtime dependencies.
"""

from __future__ import annotations

import sys
from importlib.util import find_spec


MIN_PYTHON = (3, 11)


class ProvisioningError(RuntimeError):
    pass


def check_python() -> None:
    """Ensure the current Python version is supported."""
    if sys.version_info < MIN_PYTHON:
        required = ".".join(str(v) for v in MIN_PYTHON)

        raise ProvisioningError(
            f"caddev requires Python {required} or newer; "
            f"running Python {sys.version_info.major}.{sys.version_info.minor}"
        )


def check_build123d() -> None:
    """Ensure build123d is installed in the current environment."""
    if find_spec("build123d") is None:
        raise ProvisioningError(
            "build123d is not installed in the current Python environment"
        )


def check_environment() -> None:
    """Validate the environment required to run a build123d project."""
    check_python()
    check_build123d()
