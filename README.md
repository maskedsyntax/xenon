# Xenon Text Editor

A modern, lightweight text and code editor built with C++17 and GTK 3. Xenon combines the simplicity of Kate with the user experience of Visual Studio Code.

## Features

Core Editing
- Syntax highlighting for 100+ languages via GtkSourceView 3.0
- Code completion with context-aware suggestions
- Search and replace with regular expression support
- Code formatting and linting integration
- Undo and redo operations

User Interface
- Tabbed multi-file editing
- Split pane editing (horizontal and vertical)
- File explorer sidebar with directory tree
- Quick file opening with fuzzy search (Ctrl+P)
- Settings manager with JSON configuration
- Light and dark themes

Advanced Features
- Built-in terminal emulator
- Git integration (status, commit, branch, diff)
- Plugin system for extensibility
- Auto-save and crash recovery
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

## Architecture

Xenon follows a layered architecture with clear separation of concerns:

- Core Layer: Framework-agnostic data structures and business logic
- Features Layer: Search, formatting, linting, code completion
- Services Layer: Settings management, Git integration, plugins
- UI Layer: GTK-based user interface

## Project Structure

```
src/
├── core/              # Data structures and core logic
├── features/          # Search, formatting, linting
├── services/          # Settings, Git, plugins
└── ui/               # GTK user interface
```

## Code Standards

This project follows strict production-grade code standards:

- Modular design with single responsibility per module
- No global mutable state
- Comprehensive error handling
- Security by default
- Minimal external dependencies
- Extensive testing of core features

## License

MIT License

## Development Status

Active development. Implementing core features and establishing foundation for future extensions.
