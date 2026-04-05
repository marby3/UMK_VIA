# UIAPduino_VIA

UIAPduino_VIA is an open-source, web-based configurator and firmware generator for custom keyboards.
It aims to provide a complete, browser-first experience for writing firmware, configuring key layouts, and testing keyboard behavior without needing to install dedicated desktop applications or toolchains.

## Features (In Development)

- **Web Flasher**: Directly flash firmware from your browser using WebUSB.
- **Key Mapping**: Visually configure your keyboard layout and save it to the device in real-time.
- **Key Tester**: Test each switch and make sure your keyboard is fully functional.
- **Firmware Builder (Dev)**: Generate custom firmware setups within the browser.
- **Layout Editor (Dev)**: Manage layout definitions intuitively.

## Project Structure

- `firmware/` - C source code for the microcontroller firmware (based on ch32v003fun).
- `web/` - Front-end application for the UIAPduino_VIA configurator.
- `docs/` - Project documentation and development plans.

## Local Firmware Build (For Learning & Contributing)

While this project focuses on web-based compilation, you can compile the firmware locally.

### Requirements
- **RISC-V Toolchain**: `riscv-none-elf-gcc`
- **ch32v003fun**: Required headers and build framework (included as git submodules or copied in `firmware/`).
- **Make**: Standard build tool.

### Build Instructions
1. Navigate to the firmware directory:
   ```bash
   cd firmware
   ```
2. Build the default firmware target:
   ```bash
   make
   ```
3. The resulting `main.hex` or `main.bin` can be flashed using your preferred CH32V003 flasher (e.g., WCH-LinkE).

## Contributing

We welcome contributions! Please see `CONTRIBUTING.md` for more details.

## License

This project is open-source. Please see the LICENSE file for details.
