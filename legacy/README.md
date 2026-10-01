# TVLive – M3U Live TV Player for Anbernic Handheld Consoles

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Python 3.9+](https://img.shields.io/badge/python-3.9+-blue.svg)](https://www.python.org/downloads/)

TVLive is a live TV player designed for Anbernic handheld consoles (RGxx, RGDS, etc.). It features a full‑screen GUI built with SDL2 + PIL, supporting M3U playlist parsing, grouping, multi‑line (alternative streams) switching, caching, and full‑screen playback. Place your `.m3u` files in the `TV` folder and enjoy watching IPTV on your device.

<img width="640" height="480" alt="screenshot_20260826_143432" src="https://github.com/user-attachments/assets/faddbb2c-9871-4c18-bccf-e87c7c94ba7a" />

---

## ✨ Features

- 📺 **M3U source scanning** – Automatically scans `/roms/TV`, `/mnt/mmc/TV`, `/mnt/sdcard/TV`, and the `TV` folder in the program directory for `.m3u`, `.m3u8`, and `.txt` files.
- 🗂️ **Group & multi‑line support** – Parses `group-title` and `tvg-logo`, automatically detects alternative streams for the same channel (`lineNo/lineTotal`).
- 🚀 **Caching** – Generates cache files after the first parse to speed up subsequent launches.
- 🎮 **Full‑screen playback** – Press `A` to play the selected channel in full‑screen mode; press `B` to stop and return to the list.
- 🔊 **Volume control** – Hardware (ALSA) and mpv software volume linked, adjustable with `L2`/`R2` or `V-`/`V+`.
- 🌐 **Multi‑language UI** – Built‑in languages: Simplified Chinese, Traditional Chinese, English, Japanese, Korean, French, German, Russian, Spanish, Portuguese. Press `SELECT` to switch instantly.
- 🔋 **System status display** – Real‑time WiFi connection status, battery percentage, and charging state.
- 💾 **Auto‑save** – Saves the current source, channel, and volume on exit; resumes on next launch.

---

## 📦 Dependencies & Installation

### System Requirements
- Python 3.9 or higher
- SDL2 library (`libsdl2-dev`)
- mpv player (`mpv`)
- PIL (Pillow) – automatically installed by the program (via `module.zip`)

### Installation Steps
1. Copy the project code to your Anbernic device (e.g., `/mnt/mmc/Apps/TVLive`).
2. Place your `.m3u` playlist files into any `TV` folder (e.g., `/mnt/mmc/TV` or `/roms/TV`).
3. Run `python3 tv.py` (the first run will automatically extract `module.zip` to install dependencies).
4. Use the buttons to browse and play.

---

## 🎮 Button Controls

| Button       | Action                                 |
|--------------|----------------------------------------|
| `↑` / `↓`    | Previous / Next channel                |
| `←` / `→`    | Page up / Page down                    |
| `L1` / `R1`  | Previous / Next source file            |
| `X`          | Same as `L1` – switch source           |
| `A`          | Play the selected channel (full‑screen)|
| `B`          | Stop playback, return to list          |
| `Y`          | Refresh source list                    |
| `SELECT`     | Switch UI language                     |
| `M` (MENUF)  | Exit the program                       |
| `V-` / `V+`  | Volume down / up (active during play)  |

---

## 📁 Directory Structure

```
TVLive/
├── tv.py                 # Main program
├── module.zip            # Packaged dependencies (SDL2 + PIL)
├── font/
│   └── font.ttf          # Optional custom font
├── lang/                 # Language files (.json)
│   ├── en_US.json
│   ├── zh_CN.json
│   └── ...
├── TV/                   # Default folder for .m3u sources
├── volume.txt            # Saved volume (auto‑generated)
├── tv.ini                # Configuration file (auto‑generated)
├── .cache_*.dat          # Cache files (auto‑generated)
└── tv.srt                # Temporary subtitle file (auto‑generated)
```

---

## ⚙️ Configuration

The program automatically creates `tv.ini` in the root directory with the following sections:

```ini
[General]
language = zh_CN          # UI language code

[Volume]
value = 80                # Volume (0–130)

[Resume]
source_index = 1          # Last played source index (1‑based)
channel_index = 1         # Last played channel index (1‑based)
```

You can edit this file manually to change the default language or volume.

---

## 🛠️ Developer Guide

### Manual Dependency Installation
If auto‑installation fails, you can install them manually:

```bash
pip install Pillow
apt-get install libsdl2-dev mpv
```

### Debugging
Logs are written to `tv.log`. Check this file for troubleshooting.

### Adding a New Language
1. Create a new JSON file in `lang/` (e.g., `fr_FR.json`).
2. Copy the content of `en_US.json` and translate all values.
3. Add the new language code to the `TVApp.system_langs` tuple.
4. Restart the program and press `SELECT` to switch.

---

## 🤝 Contributing

Issues and Pull Requests are welcome! Please follow these guidelines:

- Code style: PEP 8.
- Test your changes before submitting.
- For new features, update the README accordingly.

See [CONTRIBUTING.md](CONTRIBUTING.md) for more details.

---

## 📄 License

This project is open‑sourced under the [MIT License](LICENSE). You are free to use, modify, and distribute it.

---

## 🙏 Acknowledgements

- [SDL2](https://www.libsdl.org/) – Graphics rendering
- [Pillow](https://python-pillow.org/) – Image processing
- [mpv](https://mpv.io/) – Media playback

---

## 📧 Contact

For issues, please open an [Issue](https://github.com/yourusername/tvlive/issues) or contact the author.

---

**Happy Watching!** 📺

These English documents are ready to be placed in your GitHub repository. Replace `yourusername` and the repository name with your actual GitHub info. If you also need the UI translation strings translated to English, please let me know (though they are already in English by default).
