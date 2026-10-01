# TVLive for Anbernic H700

A high-performance live TV streaming application for Anbernic H700 handheld devices (RG35XX Plus series and compatible models).

## Overview

TVLive is an IPTV player designed specifically for Anbernic H700 handhelds. It supports M3U playlists, channel logo display, multi-language UI, and is optimized for low-memory embedded devices.

## Features

- 📺 **M3U Playlist Support** — Load standard M3U/M3U8 playlists
- 🖼️ **Channel Logos** — Async logo loading with disk cache
- 🌐 **Multi-language** — 10 languages supported (English, Chinese, Japanese, Korean, etc.)
- 📱 **Touch & Buttons** — Full touchscreen and gamepad button support
- 🔊 **Volume Control** — Via mpv IPC, works during playback
- 💾 **Resume** — Remembers last channel, source, and volume
- 🎨 **Dark / Light Theme** — Toggle anytime with L2

## Requirements

### Build Dependencies (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config \
    libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-gfx-dev \
    libminizip-dev libtinyxml2-dev
 
 
 将 GitHub 上的 Python 项目替换为 C++ 版本，核心思路是：**保留原有的部署逻辑，但将运行入口从 Python 脚本改为 C++ 二进制程序**。我已经根据你提供的项目地址和 H700 平台特性，为你规划好了完整的替换流程。

## 📋 完整替换流程

### 第一步：重构 GitHub 仓库文件结构

建议将仓库调整为以下结构，将 C++ 源码、资源文件和部署脚本清晰地组织起来：

```
TVLive_for_Anbernic/
├── src/                     # C++ 源代码
│   ├── main.cpp
│   ├── app.cpp
│   ├── app.hpp
│   ├── config.cpp
│   ├── config.hpp
│   ├── epub.cpp
│   ├── epub.hpp
│   ├── html_text.cpp
│   ├── html_text.hpp
│   ├── input.cpp
│   ├── input.hpp
│   ├── logo_loader.cpp
│   ├── logo_loader.hpp
│   ├── player.cpp
│   ├── player.hpp
│   ├── quick_actions.cpp
│   ├── quick_actions.hpp
│   ├── renderer.cpp
│   ├── renderer.hpp
│   ├── scanner.cpp
│   ├── scanner.hpp
│   ├── spy.cpp
│   ├── spy.hpp
│   ├── theme.cpp
│   ├── theme.hpp
│   ├── touch.cpp
│   ├── touch.hpp
│   └── translator.cpp
│   └── translator.hpp
├── lang/
│   └── lang.json            # 多语言文件
├── third_party/
│   └── nlohmann/
│       └── json.hpp
├── CMakeLists.txt
├── build.sh                 # 一键编译脚本
├── README.md                # 英文版说明文档
└── LICENSE
```

### 第二步：创建一键编译脚本 `build.sh`

在仓库根目录创建 `build.sh`：

```bash
#!/bin/bash
set -e

echo "=== TVLive C++ Builder ==="

# Check dependencies
if ! pkg-config --exists sdl2; then
    echo "Error: SDL2 development files not found."
    echo "Install: sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-gfx-dev"
    exit 1
fi

# Create build directory
mkdir -p build
cd build

# Configure and build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

echo ""
echo "=== Build complete ==="
echo "Binary: build/hud"
echo ""
echo "Next steps:"
echo "  1. Copy 'hud' and 'lang/lang.json' to your device"
echo "  2. Replace the Python 'spy.py' hook in RA_launch.sh with:"
echo "     /path/to/hud spy \"\${GUIDES_ARR[@]}\" --rom \"\$ROMFILE\" &"
```

### 第三步：编写 C++ 项目 `CMakeLists.txt`

基于你现有的 C++ 项目，创建适配的 `CMakeLists.txt`：

```cmake
cmake_minimum_required(VERSION 3.16)
project(TVLive CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
pkg_check_modules(SDL2_GFX REQUIRED SDL2_gfx)

find_library(MINIZIP_LIB NAMES minizip REQUIRED)
find_path(MINIZIP_INCLUDE_DIR NAMES unzip.h PATH_SUFFIXES minizip REQUIRED)
find_library(TINYXML2_LIB NAMES tinyxml2 REQUIRED)
find_path(TINYXML2_INCLUDE_DIR NAMES tinyxml2.h REQUIRED)

add_executable(hud
    src/main.cpp
    src/app.cpp
    src/spy.cpp
    src/config.cpp
    src/translator.cpp
    src/renderer.cpp
    src/input.cpp
    src/touch.cpp
    src/quick_actions.cpp
    src/theme.cpp
    src/epub.cpp
    src/html_text.cpp
    src/logo_loader.cpp
    src/player.cpp
    src/scanner.cpp
)

target_include_directories(hud PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${SDL2_GFX_INCLUDE_DIRS}
    ${MINIZIP_INCLUDE_DIR}
    ${TINYXML2_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party
)

target_link_libraries(hud
    -l:libSDL2-2.0.so.0
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    ${SDL2_GFX_LIBRARIES}
    ${MINIZIP_LIB}
    ${TINYXML2_LIB}
    pthread
)
```

### 第四步：修改设备上的 `RA_launch.sh`

在设备上找到并修改 `/mnt/mod/ctrl/RA_launch.sh`，将 Python 调用替换为 C++ 二进制调用。

将：

```bash
/mnt/mod/ctrl/spy.py --raconfig "${RACONFIG}" \
    --guides "${GAME_GUIDES}" \
    --rom "$ROMFILE" &
```

替换为：

```bash
mapfile -t GUIDES_ARR <<< "${GAME_GUIDES}"
/mnt/mod/ctrl/hud spy "${GUIDES_ARR[@]}" --rom "$ROMFILE" &
```

同时，将文件末尾的清理命令：

```bash
pkill -f "spy.py"
pkill -f "hud.py"
```

替换为：

```bash
pkill -f "/mnt/mod/ctrl/hud spy"
pkill -f "/mnt/mod/ctrl/hud hud"
```

### 第五步：编写英文版 `README.md`

以下是为仓库准备的英文说明文档，你可以直接替换原有的 `README.md`：

```markdown
# TVLive for Anbernic H700

A high-performance live TV streaming application for Anbernic H700 handheld devices (RG35XX Plus series and compatible models).

## Overview

TVLive is an IPTV player designed specifically for Anbernic H700 handhelds. It supports M3U playlists, channel logo display, multi-language UI, and is optimized for low-memory embedded devices.

## Features

- 📺 **M3U Playlist Support** — Load standard M3U/M3U8 playlists
- 🖼️ **Channel Logos** — Async logo loading with disk cache
- 🌐 **Multi-language** — 10 languages supported (English, Chinese, Japanese, Korean, etc.)
- 📱 **Touch & Buttons** — Full touchscreen and gamepad button support
- 🔊 **Volume Control** — Via mpv IPC, works during playback
- 💾 **Resume** — Remembers last channel, source, and volume
- 🎨 **Dark / Light Theme** — Toggle anytime with L2

## Requirements

### Build Dependencies (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config \
    libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-gfx-dev \
    libminizip-dev libtinyxml2-dev
```

### Runtime Dependencies (on device)

All required shared libraries are pre-installed on Anbernic H700 devices:

- SDL2 (2.0.12+)
- SDL2_image (2.0.9+)
- SDL2_ttf
- SDL2_gfx
- mpv (for video playback)

## Building

```bash
git clone https://github.com/cbepx-me/TVLive_for_Anbernic.git
cd TVLive_for_Anbernic
./build.sh
```

The binary will be at `build/hud`.

## Usage

### Playlists

Place your M3U/M3U8 files in `/mnt/mmc/TV/`, `/mnt/sdcard/TV/`, or `/roms/TV/`.

### Controls

| Button | Action |
|--------|--------|
| ↑ / ↓ | Channel up/down |
| ← / → | Page up/down |
| A | Play selected channel |
| B | Stop playback / Exit |
| L1 / R1 | Previous / Next source |
| L2 | Toggle theme |
| L3 | Cycle font size |
| SELECT | Exit |
| MENUF | Exit |
| V+ / V- | Volume up/down |

## Project Structure

```
src/                    — C++ source files
lang/lang.json          — Translation strings
third_party/            — Bundled third-party headers (nlohmann/json)
CMakeLists.txt          — Build configuration
build.sh                — One-click build script
```

## Technical Notes

### GPU Driver Compatibility

The H700 uses a Mali G31 GPU. SDL2's `SDL_RENDERER_ACCELERATED` may fail on some firmware versions. The application automatically falls back to software rendering if hardware acceleration is unavailable.

### Cross-Compilation

The device runs Ubuntu 22.04 LTS with glibc 2.35. To build binaries compatible with the device:

- Use GCC ≤ 12 to avoid GLIBCXX version mismatches
- Link against glibc ≤ 2.35
- A native aarch64 build (inside an arm64 Docker container) produces working binaries

### mpv IPC

Volume control is implemented via mpv's JSON IPC interface. The socket path is `/tmp/tv-mpv-<pid>.sock`.

## License

MIT License

## Credits

- Original Python implementation: cbepx-me
- C++ port: cbepx-me
- SDL2, SDL2_image, SDL2_ttf, SDL2_gfx
- nlohmann/json
- minizip
- tinyxml2
```

