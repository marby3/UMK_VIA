#!/usr/bin/env python3
"""umk - UMK_VIA build CLI.

This mimics QMK's command shape (``compile -kb <board> -km <keymap>``) so the
muscle memory carries over, but it is NOT qmk: the real QMK CLI assumes the
QMK Firmware tree and has no CH32V003 support.

All build logic lives in firmware/Makefile. This CLI only validates arguments
and shells out, so the Makefile stays the single source of truth.
"""

import argparse
import os
import shutil
import subprocess
import sys

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FIRMWARE_DIR = os.path.join(REPO_ROOT, "firmware")
BUILD_DIR = os.path.join(REPO_ROOT, "build")
KEYBOARDS_DIR = os.path.join(REPO_ROOT, "keyboards")

DEFAULT_KEYMAP = "default"


def die(message):
    sys.exit("umk: error: %s" % message)


def available_keyboards():
    if not os.path.isdir(KEYBOARDS_DIR):
        return []
    return sorted(
        name
        for name in os.listdir(KEYBOARDS_DIR)
        if os.path.isfile(os.path.join(KEYBOARDS_DIR, name, "config.h"))
    )


def available_keymaps(keyboard):
    root = os.path.join(KEYBOARDS_DIR, keyboard, "keymaps")
    if not os.path.isdir(root):
        return []
    return sorted(
        name
        for name in os.listdir(root)
        if os.path.isfile(os.path.join(root, name, "keymap.c"))
    )


def validate_keyboard(keyboard):
    if not keyboard:
        die("-kb/--keyboard is required")
    path = os.path.join(KEYBOARDS_DIR, keyboard)
    if not os.path.isdir(path):
        die(
            "no such keyboard: %s\n  available: %s"
            % (keyboard, ", ".join(available_keyboards()) or "(none)")
        )
    if not os.path.isfile(os.path.join(path, "config.h")):
        die("keyboards/%s exists but has no config.h" % keyboard)
    return keyboard


def validate_keymap(keyboard, keymap):
    path = os.path.join(KEYBOARDS_DIR, keyboard, "keymaps", keymap)
    if not os.path.isdir(path):
        die(
            "no such keymap: keyboards/%s/keymaps/%s\n  available: %s"
            % (keyboard, keymap, ", ".join(available_keymaps(keyboard)) or "(none)")
        )
    if not os.path.isfile(os.path.join(path, "keymap.c")):
        die("keyboards/%s/keymaps/%s has no keymap.c" % (keyboard, keymap))
    return keymap


def find_make():
    for candidate in ("make", "mingw32-make", "gmake"):
        found = shutil.which(candidate)
        if found:
            return found
    die("make not found on PATH")


def run_make(targets, keyboard, keymap, extra=None):
    cmd = [
        find_make(),
        "-C",
        FIRMWARE_DIR,
        "KEYBOARD=%s" % keyboard,
        "KEYMAP=%s" % keymap,
    ]
    cmd += extra or []
    cmd += targets

    print("umk: " + " ".join(cmd))
    return subprocess.call(cmd)


def cmd_compile(args):
    keyboard = validate_keyboard(args.keyboard)
    keymap = validate_keymap(keyboard, args.keymap)

    rc = run_make(["main.bin"], keyboard, keymap)
    if rc != 0:
        return rc

    binary = os.path.join(BUILD_DIR, "main.bin")
    print("umk: built %s (%s / %s)" % (binary, keyboard, keymap))
    return 0


MINICHLINK_DIR = os.path.join(
    REPO_ROOT, "firmware", "lib", "ch32v003fun", "minichlink"
)


def minichlink_path():
    for name in ("minichlink.exe", "minichlink"):
        candidate = os.path.join(MINICHLINK_DIR, name)
        if os.path.isfile(candidate):
            return candidate
    found = shutil.which("minichlink")
    return found


def cmd_flash(args):
    """Build, then write over the USB bootloader or an SWD probe.

    minichlink auto-detects both, so no flag is needed in the normal case.
    """
    keyboard = validate_keyboard(args.keyboard)
    keymap = validate_keymap(keyboard, args.keymap)

    rc = run_make(["main.bin"], keyboard, keymap)
    if rc != 0:
        return rc

    binary = os.path.join(BUILD_DIR, "main.bin")

    if not minichlink_path():
        print(
            "\n"
            "umk: minichlink is missing, so `umk flash` cannot write the chip.\n"
            "     Built firmware is ready at: %s\n"
            "     Flash it to address 0x08000000 with whatever tool you use.\n"
            % binary
        )
        return 1

    extra = ["MINICHLINK_EXTRA=-C %s" % args.programmer] if args.programmer else None
    rc = run_make(["cv_flash"], keyboard, keymap, extra=extra)
    if rc != 0:
        print(
            "\n"
            "umk: flashing failed. Built firmware is still at: %s\n"
            "\n"
            "     'Could not initialize any supported programmers' means nothing\n"
            "     answered. Two ways in:\n"
            "\n"
            "     USB bootloader - no probe needed (VID 0x1209 / PID 0xB003).\n"
            "       Put the board into bootloader mode first: on most builds a\n"
            "       replug (it waits ~5s before starting user code), or holding\n"
            "       the boot button while plugging in. Then:\n"
            "         umk flash -kb %s --programmer b003boot\n"
            "\n"
            "     WCH-LinkE over SWD (SWIO -> PD1, GND, 3V3):\n"
            "         umk flash -kb %s --programmer linke\n"
            "\n"
            "     See what is actually reachable:\n"
            "       firmware/lib/ch32v003fun/minichlink/minichlink -i\n"
            % (binary, keyboard, keyboard)
        )
    return rc


def cmd_clean(args):
    keyboard = validate_keyboard(args.keyboard)
    keymap = args.keymap
    return run_make(["clean"], keyboard, keymap)


def cmd_list(args):
    keyboards = available_keyboards()
    if not keyboards:
        print("(no keyboards under keyboards/)")
        return 0
    for keyboard in keyboards:
        keymaps = available_keymaps(keyboard) or ["(no keymaps)"]
        print("%s: %s" % (keyboard, ", ".join(keymaps)))
    return 0


def build_parser():
    parser = argparse.ArgumentParser(
        prog="umk",
        description="UMK_VIA firmware build CLI (QMK-style argument syntax).",
    )
    sub = parser.add_subparsers(dest="command")

    def add_board_args(sp):
        sp.add_argument("-kb", "--keyboard", metavar="NAME")
        sp.add_argument(
            "-km",
            "--keymap",
            metavar="NAME",
            default=DEFAULT_KEYMAP,
            help="default: %s" % DEFAULT_KEYMAP,
        )

    sp = sub.add_parser("compile", help="build build/main.bin")
    add_board_args(sp)
    sp.set_defaults(func=cmd_compile)

    sp = sub.add_parser("flash", help="build and write over USB or SWD")
    add_board_args(sp)
    sp.add_argument(
        "--programmer",
        metavar="NAME",
        help="force a minichlink programmer: b003boot (USB bootloader) or "
        "linke (WCH-LinkE). Omit to auto-detect.",
    )
    sp.set_defaults(func=cmd_flash)

    sp = sub.add_parser("clean", help="remove build artefacts")
    add_board_args(sp)
    sp.set_defaults(func=cmd_clean)

    sp = sub.add_parser("list", help="list keyboards and their keymaps")
    sp.set_defaults(func=cmd_list)

    return parser


def main():
    parser = build_parser()
    args = parser.parse_args()
    if not getattr(args, "func", None):
        parser.print_help()
        return 1
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
