# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

UMK_VIA (formerly UIAPduino_VIA) is a web-based configurator and firmware generator for custom keyboards built on the WCH **CH32V003** RISC-V microcontroller. It has two halves that talk to each other over WebHID/WebUSB using the **VIA protocol**:

- `firmware/` — C firmware (bare-metal, no RTOS) built on the vendored `ch32v003fun` + `rv003usb` (software USB) frameworks.
- `web/` — a vanilla HTML/JS/CSS front end (no build step, no framework) that flashes firmware, edits keymaps live over WebHID, and generates new keyboard definitions.

There is no automated test suite in this repo; verification is manual (build the firmware, flash real hardware, exercise the web UI in Chrome/Edge).

## Commands

### Run the web configurator + local build API
```bash
python build_server.py
```
Serves `web/` at `http://localhost:8000/web/` and additionally handles `POST /api/build` (regenerates `keyboards/uiapduino/config.h` + `uiapduino.remap.json` from the builder UI's JSON config, then runs `make clean` and `make main.bin`, streaming the resulting binary back) and `POST /api/remap-definition` (JSON-only, no build). This is the primary way to exercise the "Keyboard Builder" (dev) tab end-to-end.

For pure front-end iteration without the build API, `cd web && python -m http.server 8000` also works.

### Build firmware directly
```bash
cd firmware
make KEYBOARD=uiapduino main.bin   # build only, skips the default flash target
make KEYBOARD=uiapduino            # default target also attempts cv_flash (needs a WCH-LinkE attached)
make KEYBOARD=uiapduino clean
```
`KEYBOARD` is required (`$(error ...)` if unset) and must name a directory under `keyboards/`. It pulls in `keyboards/<name>/rules.mk` and, if present, `keyboards/<name>/keymap.c`.

Requires the `riscv-none-elf-gcc` toolchain and the `firmware/ch32v003fun` and `firmware/rv003usb` git submodules (`git submodule update --init --recursive`).

## Architecture

### Firmware (`firmware/`)

`main.c` runs the whole device: on each 1ms loop tick it scans the matrix, feeds edge events into the keymap layer, runs the VIA/flash/RGB housekeeping tasks, and rebuilds the USB report only when something changed.

- **`matrix.c`/`.h`** — GPIO matrix (or direct-pin) scanning. Defines `MATRIX_ROWS`/`MATRIX_COLS` from `CUSTOM_MATRIX_ROWS`/`CUSTOM_MATRIX_COLS`, and `LOGICAL_ROWS`/`LOGICAL_COLS` — the split-combined size that VIA/Remap actually sees (doubled in rows or cols depending on `CUSTOM_SPLIT_COMBINE_COLS`/`ROWS`).
- **`keymap.c`/`.h`** — QMK-compatible 16-bit keycode interpretation (basic keys, mod+key, Mod-Tap, Layer-Tap, `TO`/`MO`/`DF`/`TG` layer switching), evaluated top layer down with `KC_TRNS` fallthrough. Tap/Hold supports only one pending key at a time. `LAYERS` defaults to 4 (override via `CUSTOM_LAYERS`); RAM cost is `LAYERS * LOGICAL_ROWS * LOGICAL_COLS * 2` bytes against a 2KB budget — watch this when changing matrix size or layer count.
- **`via.c`/`.h`** — VIA protocol v12 (`0x000C`) implementation, the bridge to Remap and to `web/protocol.js`. Because `rv003usb` ACKs the host immediately after the OUT-endpoint interrupt handler returns, command handling is deliberately split: `via_receive_packet()` (ISR) only assembles 8-byte USB packets into the 32-byte VIA report; `via_task()` (main loop) does the actual command interpretation and response building; `via_handle_in()` (ISR) drains the prepared response 8 bytes at a time, always returning an empty packet when nothing is pending (Remap disconnects on an unsolicited report). Don't move slow work into the ISR-side functions.
- **`flash_store.c`/`.h`** — persists the keymap to the last page(s) of the 16KB internal flash (no EEPROM on this MCU). Autosaves `FLASH_AUTOSAVE_DELAY_MS` (750ms) after VIA traffic goes quiet, writes in 64-byte pages to bound interrupt latency, and never reboots after saving (so the Remap/WebHID connection stays alive). A `"VKM1"` magic + stored layer/row/col counts guard against loading stale data after a matrix-size change.
- **`split.c`/`.h`** — split keyboard support over USART1 (Tx `PD5`/Rx `PD6`, fixed). Master/slave role is auto-negotiated at boot by whether USB enumerates within 500ms; the master merges the slave's local matrix into the logical `global_matrix_state`.
- **`usb_config.h`/`.c`** — USB descriptors for 3 endpoints: EP0 control, EP1 (boot keyboard HID, 6KRO), EP2 (VIA Raw HID, Usage Page `0xFF60`/Usage `0x61`, 32-byte reports). Because this is Low-Speed USB, packets are capped at 8 bytes — the 32-byte VIA report is chunked 8×4, which is why `via.c` and `web/protocol.js` both deal in packet math.
- **`rgb_led.c`/`.h`** — optional RGB effects, gated by `CUSTOM_RGB_ENABLE`.
- **`ch32v003fun/`, `rv003usb/`** — vendored git submodules; treat as third-party, don't edit.

Board-specific configuration flows through `CUSTOM_*` macros defined in `keyboards/<name>/config.h` (pulled in everywhere via `__has_include("config.h")`, resolved through the `-I$(KEYBOARD_DIR)` the Makefile adds). This file, plus `keyboards/<name>/uiapduino.remap.json`, is **auto-generated/overwritten by `build_server.py`** from whatever the web builder UI submits — don't hand-edit expecting changes to survive a rebuild; change the web UI or `build_server.py`'s generator instead. `firmware/custom_config.h` is a similar-looking generated artifact but is not currently `#include`d anywhere.

### Keyboard definitions (`keyboards/<name>/`)

Per-board `config.h` (pins/matrix/features), `rules.mk` (build flags), optional `keymap.c` (default keymap override — the `default_keymap` symbol is `weak` in firmware so a board can supply its own), and `<name>.remap.json` (VIA keyboard definition for registering with remap-keys.app). Currently only `keyboards/uiapduino/` exists.

### Web front end (`web/`)

No bundler/build step — plain scripts loaded from `index.html`.

- **`protocol.js`** — `UIAPduinoProtocol` class: WebHID client speaking the same VIA command set as `firmware/via.c` (command IDs, buffer chunking at 28 bytes/command, single in-flight command queue, echo-based response matching).
- **`hid_app.js`** — the "キーマッピング" (keymap editor) view; connects via `protocol.js`, renders the layout, reads/writes keycodes and layers, triggers flash save/reset.
- **`builder_app.js`** — the "開発ユーザー" (dev) tab: hardware/keyboard builder (matrix vs. direct-pin wiring, pin assignment, split config, RGB config) that POSTs to `build_server.py`'s `/api/build`, plus a drag/resize layout editor (assigns each visual key a matrix row/col) that can export a plain keyboard-definition JSON or a Remap-ready definition (via `/api/remap-definition` or client-side `generateRemapDefinition()`).
- **`flasher_app.js`** — WebUSB flasher ported from `rv003usb-webflasher`, talks to the CH32V003 bootloader directly (no drivers) to write a `.bin` to flash from the browser.
- **`tester_app.js`** — key tester view (visual per-key input feedback).

`index.html` is a single page with view-switching tabs (flasher / keymap / tester / dev / settings / links) rather than separate routes.

### `build_server.py`

The bridge between the web builder UI and the local toolchain: generates `keyboards/uiapduino/config.h` + `uiapduino.remap.json` from a JSON config (mirroring the size/split logic in `matrix.h`, see `logical_matrix_size()`), then shells out to `make -C firmware KEYBOARD=uiapduino`. If you change how `matrix.h` computes `LOGICAL_ROWS`/`LOGICAL_COLS` or how `CUSTOM_*` macros are named, update this file's generator in lockstep or the two will silently diverge.

### Docs (`docs/`)

`docs/` contains exactly one file, **`specification.md`** — the single authoritative master spec and
the blueprint for the from-scratch rewrite. All prior planning/spec docs (`firmware_specification.md`,
`remap_usage.md`, `uiapduino_via_development/`, `advanced_development_phases/`, `keyboard_build_structure/`)
were deleted because they caused confusion once `specification.md` superseded them — do not recreate them;
if historical context is needed, it's in git history (`git log -- docs/`).

Key decisions codified in `specification.md`:

- Project renamed `UIAPduino_VIA` → **`UMK_VIA`** (the sample board keeps its own name, `keyboards/uiapduino/`).
- The web UI drops its own keymap-editing screen entirely and defers all keymap editing to Remap
  (remap-keys.app) — the web UI's job is limited to the hardware/keyboard builder, the web flasher,
  the key tester, and generating Remap-ready keyboard definitions.
- Building should also be possible via a from-scratch CLI named `umk` that mimics QMK's command rules
  (`umk compile -kb <name> -km <keymap>`) — not the real `qmk` pip package, which assumes QMK Firmware's
  own build layout and doesn't support CH32V003. This requires splitting `keyboards/<name>/keymap.c`
  into `keyboards/<name>/keymaps/<keymap>/keymap.c` to support multiple keymaps per board.
- Documents split/RGB wire-protocol details that were previously undocumented, and calls out known bugs
  to fix during the rewrite (`CUSTOM_RGB_PIN` generated by `build_server.py` but never read by `rgb_led.c`;
  an undefined `UNIT` variable reference in `builder_app.js`'s canvas renderer).

None of this is implemented yet — the current `firmware/`, `web/`, and `build_server.py` described above
are still the pre-rewrite `UIAPduino_VIA` code and match the rest of this file, not `specification.md`.
