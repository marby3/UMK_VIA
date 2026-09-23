#!/usr/bin/env python3
"""Write a stamp file, but only when its contents actually change.

Used by firmware/Makefile to record which keyboard/keymap the current build/
belongs to. Rewriting unconditionally would relink on every invocation;
leaving it alone would hand back a stale binary after `-kb`/`-km` changed.
"""

import os
import sys


def main():
    if len(sys.argv) < 3:
        sys.exit("usage: stamp.py <path> <content>...")

    path = sys.argv[1]
    content = " ".join(sys.argv[2:])

    parent = os.path.dirname(path)
    if parent:
        os.makedirs(parent, exist_ok=True)

    if os.path.exists(path):
        with open(path, encoding="utf-8") as handle:
            if handle.read() == content:
                return 0

    with open(path, "w", encoding="utf-8") as handle:
        handle.write(content)
    return 0


if __name__ == "__main__":
    sys.exit(main())
