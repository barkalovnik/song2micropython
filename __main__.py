"""Точка входа: `python -m song2notes` — GUI, с аргументами — командная строка."""
from __future__ import annotations

import sys

def main() -> int:
    if len(sys.argv) > 1:
        from .cli import main as cli_main
        return cli_main(sys.argv[1:])
    from .gui.app import run
    return run()


if __name__ == "__main__":
    raise SystemExit(main())
