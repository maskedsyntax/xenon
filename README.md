# Xenon Text Editor

A modern, lightweight text and code editor built with C++17 and GTK 3. Xenon combines the simplicity of Kate with the user experience of Visual Studio Code.

## Features

Core Editing
- Syntax highlighting for 100+ languages via GtkSourceView 3.0
- Search and replace with regular expression support
- Undo and redo operations

User Interface
- Tabbed multi-file editing
- Split pane editing (horizontal and vertical)
- File explorer sidebar with directory tree
- Quick file opening with fuzzy search (Ctrl+P)
- Light and dark themes

Advanced Features
- Cross-platform support (Linux, Windows, macOS)

## Requirements

- C++17 compatible compiler (GCC 7+, Clang 6+, MSVC 2019+)
- CMake 3.10 or later
- GTK 3.22 or later
- GtkSourceView 3.0
- VTE 2.91
- libgit2

## Installation

### Ubuntu/Debian

```bash
sudo apt-get install \
  libgtk-3-dev \
  libgtksourceview-3.0-dev \
  libvte-2.91-dev \
  libgtkmm-3.0-dev \
  libgtksourceviewmm-3.0-dev \
  libgit2-dev \
  pkg-config
```

## Building

```bash
mkdir build
cd build
cmake ..
make
```

The executable will be created at `build/bin/xenon`.

## Running

```bash
./build/bin/xenon
```

## License

MIT License

## Development Status

Active development. Implementing core features and establishing foundation for future extensions.