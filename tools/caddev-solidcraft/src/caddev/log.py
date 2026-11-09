"""Colored terminal logging for caddev."""

from __future__ import annotations

import sys
from datetime import datetime


RESET = "\033[0m"
BOLD = "\033[1m"

RED = "\033[31m"
GREEN = "\033[32m"
YELLOW = "\033[33m"
BLUE = "\033[34m"
CYAN = "\033[36m"
GRAY = "\033[90m"


def _time() -> str:
    return datetime.now().strftime("%H:%M:%S")


def _write(
    symbol: str,
    message: str,
    color: str,
    *,
    stream=sys.stdout,
) -> None:
    print(
        f"{GRAY}{_time()}{RESET} "
        f"{color}{BOLD}{symbol}{RESET} "
        f"{message}",
        file=stream,
        flush=True,
    )


def info(message: str) -> None:
    _write("●", message, BLUE)


def success(message: str) -> None:
    _write("✓", message, GREEN)


def warning(message: str) -> None:
    _write("!", message, YELLOW)


def error(message: str) -> None:
    _write("✗", message, RED, stream=sys.stderr)


def build(message: str) -> None:
    _write("◆", message, CYAN)
