# Contributing to UIAPduino_VIA

Thank you for your interest in contributing to UIAPduino_VIA! This project is being built to provide an accessible and powerful tool for keyboard creators and enthusiasts.

## How to Contribute

1. **Bug Reports**: If you find an issue, please open an issue in the GitHub repository matching your observation. Include browser version, OS, and microcontroller details if relevant.
2. **Feature Requests**: We welcome ideas! Open an issue to discuss new features.
3. **Pull Requests**:
   - Fork the repository.
   - Create a feature branch (`git checkout -b feature/your-feature-name`).
   - Commit your changes with clear, descriptive messages.
   - Push to your branch and open a Pull Request.

## Development Setup

### Front-End (Web)
The front-end currently uses standard Web technologies (HTML, CSS, JS). You can run a local server to test UI changes.
```bash
cd web
python -m http.server 8000
```
Then visit `http://localhost:8000`.

### Firmware
For firmware modifications, follow the build instructions in the `README.md`. Test your changes thoroughly on hardware to ensure the WebUSB integration remains stable.

## Code of Conduct
Please be respectful and patient when interacting with other contributors and users.
