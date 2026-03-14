# Xenon Text Editor (Qt 6 Edition)

A modern, lightweight, and fast text editor inspired by Zed and VS Code, built with C++17 and Qt 6.

## Features

- **Native macOS Look:** Unified title and toolbar support.
- **Zed-like Layout:** Vertical Activity Bar and collapsible Sidebar.
- **Tabbed Editor:** High-performance code editor with line numbers.
- **Syntax Highlighting:** Initial support for C++ and general code.
- **Integrated Terminal:** Real-time shell integration.
- **Command Palette:** Quick access to commands via `Cmd+Shift+P`.
- **Git Integration:** Displays current branch in the status bar.
- **LSP Ready:** Core infrastructure for Language Server Protocol.

## Requirements

- **C++17 Compiler**
- **Qt 6.2+** (Widgets, Gui, Core, Network)
- **CMake 3.16+**
- **LibGit2** (Optional but recommended)

## Installation (macOS)

```bash
brew install qt cmake libgit2
```

## Building

```bash
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
```

## Running

```bash
# macOS Bundle
open bin/xenon.app

# Linux/Windows
./bin/xenon
```

## License
MIT
