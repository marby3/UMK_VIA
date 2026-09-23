#!/usr/bin/env python3
"""Verify the linked firmware does not overlap the keymap store.

The keymap is persisted into the last pages of the CH32V003's 16KB flash, and
nothing in the linker script reserves that region. If .text grows past the
store's start address the first VIA save corrupts the firmware, so this check
runs on every build.

flash_store.c exports the reserved size as the absolute symbol
``__umk_flash_store_reserved``; the firmware size comes from the section
headers of the ELF.
"""

import argparse
import re
import subprocess
import sys

FLASH_TOTAL = 16 * 1024
# The CH32V003 maps its flash both at 0x08000000 and, aliased, at 0x00000000.
# ch32fun's linker script links against the alias, so accept either base.
FLASH_ALIAS = 0x08000000
RESERVED_SYMBOL = "__umk_flash_store_reserved"


def run(cmd):
    try:
        return subprocess.run(
            cmd, check=True, capture_output=True, text=True
        ).stdout
    except FileNotFoundError:
        sys.exit("flash_guard: cannot run %s - is the toolchain on PATH?" % cmd[0])
    except subprocess.CalledProcessError as exc:
        sys.exit("flash_guard: %s failed:\n%s" % (" ".join(cmd), exc.stderr))


def reserved_bytes(prefix, elf):
    out = run(["%s-nm" % prefix, elf])
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[2] == RESERVED_SYMBOL:
            return int(parts[0], 16)
    sys.exit(
        "flash_guard: %s not found in %s - flash_store.c did not get linked in"
        % (RESERVED_SYMBOL, elf)
    )


def firmware_bytes(prefix, elf):
    """Highest flash address touched by any allocated, loadable section."""
    out = run(["%s-objdump" % prefix, "-h", elf])
    top = 0
    # Idx Name Size VMA LMA File-off Algn, followed by a flags line.
    row = re.compile(
        r"^\s*\d+\s+(\S+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s", re.I
    )
    lines = out.splitlines()
    for i, line in enumerate(lines):
        m = row.match(line)
        if not m:
            continue
        flags = lines[i + 1] if i + 1 < len(lines) else ""
        if "ALLOC" not in flags or "LOAD" not in flags:
            continue
        size = int(m.group(2), 16)
        lma = int(m.group(4), 16)
        if FLASH_ALIAS <= lma < FLASH_ALIAS + FLASH_TOTAL:
            lma -= FLASH_ALIAS
        elif lma >= FLASH_TOTAL:
            continue  # RAM or a non-loaded section
        top = max(top, lma + size)
    return top


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--prefix", required=True, help="toolchain prefix")
    ap.add_argument("--elf", required=True)
    args = ap.parse_args()

    reserved = reserved_bytes(args.prefix, args.elf)
    used = firmware_bytes(args.prefix, args.elf)
    limit = FLASH_TOTAL - reserved

    print(
        "Flash: %d / %d bytes used (%d B reserved for the keymap store at 0x%08X)"
        % (used, limit, reserved, FLASH_ALIAS + limit)
    )

    if used > limit:
        sys.exit(
            "flash_guard: FIRMWARE TOO LARGE - .text runs %d bytes into the "
            "keymap store. Saving a keymap would corrupt the firmware.\n"
            "Shrink the build (fewer LAYERS, disable CUSTOM_RGB_ENABLE / "
            "CUSTOM_SPLIT_ENABLE) or a smaller matrix." % (used - limit)
        )


if __name__ == "__main__":
    main()
