#!/usr/bin/env python3
"""via_probe - talk VIA to a flashed UMK_VIA board and check every command.

Run this before opening Remap. Remap silently disconnects on a malformed
report, so a failure here is much easier to read than a failure there.

    pip install hidapi
    python tools/via_probe.py                       # defaults to 0x1209/0xB803
    python tools/via_probe.py --vid 0x1209 --pid 0xb803
    python tools/via_probe.py --keyboard uiapduino  # read IDs from its config.h

The 32 byte VIA reports are split into four 8 byte Low-Speed packets by the
firmware; if that reassembly is wrong, the very first check fails.
"""

import argparse
import os
import re
import sys
import time

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

RAW_USAGE_PAGE = 0xFF60
RAW_USAGE = 0x61
REPORT_SIZE = 32

# Command IDs
GET_PROTOCOL_VERSION = 0x01
GET_KEYBOARD_VALUE = 0x02
SET_KEYBOARD_VALUE = 0x03
DYNAMIC_KEYMAP_GET_KEYCODE = 0x04
DYNAMIC_KEYMAP_SET_KEYCODE = 0x05
DYNAMIC_KEYMAP_MACRO_GET_COUNT = 0x0C
DYNAMIC_KEYMAP_MACRO_GET_BUFSZ = 0x0D
DYNAMIC_KEYMAP_GET_LAYER_COUNT = 0x11
DYNAMIC_KEYMAP_GET_BUFFER = 0x12
DYNAMIC_KEYMAP_GET_ENCODER = 0x14
UNHANDLED = 0xFF

ID_UPTIME = 0x01
ID_LAYOUT_OPTIONS = 0x02
ID_SWITCH_MATRIX_STATE = 0x03
ID_FIRMWARE_VERSION = 0x04
ID_UMK_SAVE_NOW = 0xFF

EXPECTED_PROTOCOL = 0x000C


class Result:
    def __init__(self):
        self.passed = 0
        self.failed = 0

    def check(self, name, ok, detail=""):
        mark = "PASS" if ok else "FAIL"
        if ok:
            self.passed += 1
        else:
            self.failed += 1
        print("  [%s] %-38s %s" % (mark, name, detail))
        return ok

    def info(self, name, detail):
        print("  [ -- ] %-38s %s" % (name, detail))


def parse_hex(value):
    return int(value, 16) if isinstance(value, str) else value


def ids_from_keyboard(name):
    """Pull CUSTOM_VID / CUSTOM_PID out of keyboards/<name>/config.h."""
    path = os.path.join(REPO_ROOT, "keyboards", name, "config.h")
    if not os.path.isfile(path):
        sys.exit("via_probe: no such keyboard config: %s" % path)
    text = open(path, encoding="utf-8", errors="replace").read()
    out = {}
    for key in ("CUSTOM_VID", "CUSTOM_PID"):
        m = re.search(r"^\s*#define\s+%s\s+(0[xX][0-9a-fA-F]+)" % key, text, re.M)
        if m:
            out[key] = int(m.group(1), 16)
    if "CUSTOM_VID" not in out or "CUSTOM_PID" not in out:
        sys.exit("via_probe: could not find CUSTOM_VID/CUSTOM_PID in %s" % path)
    return out["CUSTOM_VID"], out["CUSTOM_PID"]


def open_device(vid, pid):
    try:
        import hid
    except ImportError:
        sys.exit(
            "via_probe: the 'hid' module is missing.\n"
            "  pip install hidapi"
        )

    matches = [
        d
        for d in hid.enumerate(vid, pid)
        if d.get("usage_page") == RAW_USAGE_PAGE and d.get("usage") == RAW_USAGE
    ]
    if not matches:
        present = hid.enumerate(vid, pid)
        if present:
            sys.exit(
                "via_probe: found %04X:%04X but no interface with usage page "
                "0x%04X / usage 0x%02X.\n"
                "  Remap looks for exactly that pair, so it would not see this\n"
                "  device either. Check the raw HID report descriptor.\n"
                "  Interfaces seen: %s"
                % (
                    vid,
                    pid,
                    RAW_USAGE_PAGE,
                    RAW_USAGE,
                    ", ".join(
                        "usage_page=0x%04X usage=0x%02X"
                        % (d.get("usage_page", 0), d.get("usage", 0))
                        for d in present
                    ),
                )
            )
        sys.exit(
            "via_probe: no device %04X:%04X found. Is it plugged in and running "
            "UMK_VIA?" % (vid, pid)
        )

    dev = hid.Device(path=matches[0]["path"])
    return dev, matches[0]


class Via:
    def __init__(self, dev, timeout_ms=1000):
        self.dev = dev
        self.timeout_ms = timeout_ms

    def xfer(self, payload):
        """Send a 32 byte command, return the 32 byte reply."""
        buf = bytearray(REPORT_SIZE)
        buf[: len(payload)] = bytes(payload)
        # hidapi wants a leading report ID; VIA uses unnumbered reports, so 0.
        self.dev.write(b"\x00" + bytes(buf))
        reply = self.dev.read(REPORT_SIZE, timeout=self.timeout_ms)
        if not reply:
            raise TimeoutError("no reply to command 0x%02X" % payload[0])
        return bytes(reply)


def be32(data, offset=0):
    return int.from_bytes(data[offset : offset + 4], "big")


def run(via, res, do_write):
    # --- protocol version ------------------------------------------------
    r = via.xfer([GET_PROTOCOL_VERSION])
    proto = (r[1] << 8) | r[2]
    res.check(
        "get_protocol_version",
        r[0] == GET_PROTOCOL_VERSION and proto == EXPECTED_PROTOCOL,
        "0x%04X (expected 0x%04X)" % (proto, EXPECTED_PROTOCOL),
    )

    # --- layer count -----------------------------------------------------
    r = via.xfer([DYNAMIC_KEYMAP_GET_LAYER_COUNT])
    layers = r[1]
    res.check("dynamic_keymap_get_layer_count", 1 <= layers <= 16, "%d layers" % layers)

    # --- uptime ----------------------------------------------------------
    r1 = via.xfer([GET_KEYBOARD_VALUE, ID_UPTIME])
    t1 = be32(r1, 2)
    time.sleep(0.25)
    r2 = via.xfer([GET_KEYBOARD_VALUE, ID_UPTIME])
    t2 = be32(r2, 2)
    res.check(
        "get_keyboard_value(uptime) advances",
        t2 > t1,
        "%d ms -> %d ms (+%d)" % (t1, t2, t2 - t1),
    )
    # The main loop ticks every 1ms, so ~250 counts should have elapsed. A wildly
    # different rate means the tick loop is being starved.
    res.check(
        "1ms tick rate is sane",
        150 <= (t2 - t1) <= 400,
        "%d ticks over 250ms" % (t2 - t1),
    )

    # --- firmware version ------------------------------------------------
    r = via.xfer([GET_KEYBOARD_VALUE, ID_FIRMWARE_VERSION])
    res.info("get_keyboard_value(firmware_version)", "0x%08X" % be32(r, 2))

    # --- layout options round trip ---------------------------------------
    r = via.xfer([GET_KEYBOARD_VALUE, ID_LAYOUT_OPTIONS])
    res.check("get_keyboard_value(layout_options)", r[0] == GET_KEYBOARD_VALUE,
              "0x%08X" % be32(r, 2))

    # --- switch matrix state ---------------------------------------------
    r = via.xfer([GET_KEYBOARD_VALUE, ID_SWITCH_MATRIX_STATE])
    res.check(
        "get_keyboard_value(switch_matrix_state)",
        r[0] == GET_KEYBOARD_VALUE,
        "rows: " + " ".join("%02X" % b for b in r[2:14]),
    )

    # --- macros report as unavailable ------------------------------------
    r = via.xfer([DYNAMIC_KEYMAP_MACRO_GET_COUNT])
    res.check("macro_get_count == 0", r[1] == 0, "%d" % r[1])
    r = via.xfer([DYNAMIC_KEYMAP_MACRO_GET_BUFSZ])
    res.check(
        "macro_get_buffer_size == 0", (r[1] << 8 | r[2]) == 0, "%d" % (r[1] << 8 | r[2])
    )

    # --- out of scope commands answer id_unhandled ------------------------
    r = via.xfer([DYNAMIC_KEYMAP_GET_ENCODER, 0, 0, 0])
    res.check("encoder command -> id_unhandled", r[0] == UNHANDLED, "0x%02X" % r[0])

    # --- keycode reads ----------------------------------------------------
    r = via.xfer([DYNAMIC_KEYMAP_GET_KEYCODE, 0, 0, 0])
    kc00 = (r[4] << 8) | r[5]
    res.check("dynamic_keymap_get_keycode(0,0,0)", r[0] == DYNAMIC_KEYMAP_GET_KEYCODE,
              "0x%04X" % kc00)

    # --- buffer read agrees with the per-key read -------------------------
    r = via.xfer([DYNAMIC_KEYMAP_GET_BUFFER, 0x00, 0x00, 28])
    buf_kc00 = (r[4] << 8) | r[5]
    res.check(
        "get_buffer agrees with get_keycode",
        buf_kc00 == kc00,
        "buffer 0x%04X vs keycode 0x%04X" % (buf_kc00, kc00),
    )
    res.info("keymap buffer[0:16]", " ".join("%02X" % b for b in r[4:20]))

    # --- write round trip -------------------------------------------------
    if not do_write:
        res.info("set_keycode round trip", "skipped (pass --write to enable)")
        return

    probe_kc = 0x0004 if kc00 != 0x0004 else 0x0005  # KC_A / KC_B
    via.xfer([DYNAMIC_KEYMAP_SET_KEYCODE, 0, 0, 0, probe_kc >> 8, probe_kc & 0xFF])
    r = via.xfer([DYNAMIC_KEYMAP_GET_KEYCODE, 0, 0, 0])
    got = (r[4] << 8) | r[5]
    ok = res.check(
        "set_keycode -> get_keycode round trip",
        got == probe_kc,
        "wrote 0x%04X, read 0x%04X" % (probe_kc, got),
    )

    # Restore, then force the flash save so the board is left as we found it.
    via.xfer([DYNAMIC_KEYMAP_SET_KEYCODE, 0, 0, 0, kc00 >> 8, kc00 & 0xFF])
    r = via.xfer([DYNAMIC_KEYMAP_GET_KEYCODE, 0, 0, 0])
    restored = (r[4] << 8) | r[5]
    res.check("original keycode restored", restored == kc00, "0x%04X" % restored)

    via.xfer([SET_KEYBOARD_VALUE, ID_UMK_SAVE_NOW])
    time.sleep(0.2)
    # Surviving a save means the flash write did not wedge the USB stack.
    r = via.xfer([GET_PROTOCOL_VERSION])
    res.check(
        "still responsive after immediate flash save",
        r[0] == GET_PROTOCOL_VERSION,
        "replug and re-run to confirm the keymap persisted" if ok else "",
    )


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--vid", type=parse_hex, default=None)
    ap.add_argument("--pid", type=parse_hex, default=None)
    ap.add_argument(
        "--keyboard", help="read CUSTOM_VID/CUSTOM_PID from keyboards/<name>/config.h"
    )
    ap.add_argument(
        "--write",
        action="store_true",
        help="also test writing a keycode (restored afterwards)",
    )
    args = ap.parse_args()

    if args.keyboard:
        vid, pid = ids_from_keyboard(args.keyboard)
    else:
        vid = args.vid if args.vid is not None else 0x1209
        pid = args.pid if args.pid is not None else 0xB803

    print("via_probe: looking for %04X:%04X" % (vid, pid))
    dev, info = open_device(vid, pid)
    print(
        "via_probe: opened %s / %s (usage page 0x%04X, usage 0x%02X)\n"
        % (
            info.get("manufacturer_string") or "?",
            info.get("product_string") or "?",
            info.get("usage_page", 0),
            info.get("usage", 0),
        )
    )

    res = Result()
    try:
        run(Via(dev), res, args.write)
    except TimeoutError as exc:
        res.check("transfer", False, str(exc))
        print(
            "\n  A timeout here usually means the EP2 IN path never sends the\n"
            "  prepared reply - check via_handle_in() chunking in core/via.c."
        )
    finally:
        dev.close()

    print("\nvia_probe: %d passed, %d failed" % (res.passed, res.failed))
    return 1 if res.failed else 0


if __name__ == "__main__":
    sys.exit(main())
