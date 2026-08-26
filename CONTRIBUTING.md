# Contributing to TVLive

Thank you for your interest in contributing to TVLive! We welcome all kinds of contributions – bug reports, feature requests, code improvements, and documentation updates.

## How to Contribute

### Reporting Bugs
- Please check the [Issues](https://github.com/yourusername/tvlive/issues) page first to see if the problem has already been reported.
- If not, create a new Issue with:
  - Device model and system version.
  - A clear description of the problem.
  - Steps to reproduce.
  - Relevant logs (content of `tv.log`).

### Submitting Code
1. Fork this repository.
2. Create your feature branch (`git checkout -b feature/amazing-feature`).
3. Follow PEP 8 coding style.
4. Ensure your code runs correctly on the device.
5. Commit your changes (`git commit -m 'Add some amazing feature'`).
6. Push to your branch (`git push origin feature/amazing-feature`).
7. Open a Pull Request.

### Code Style
- Use 4 spaces for indentation (no tabs).
- Line length limit: 120 characters.
- Class names in CamelCase, functions and variables in snake_case.
- Add necessary comments and docstrings.

### Adding a New Language
1. Create a new JSON file in `lang/` (e.g., `fr_FR.json`), following the structure of `en_US.json`.
2. Add the new language code to the `TVApp.system_langs` tuple.
3. Test the switching functionality.

## Development Environment
- Python 3.9+
- Dependencies: SDL2, PIL, mpv

## License
All contributions will be licensed under the same MIT license as the project.
