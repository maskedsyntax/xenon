# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Configure and build
mkdir build && cd build
cmake ..
make

# Run the editor
./build/bin/xenon

# Run tests (infrastructure exists but no tests yet)
cd build && ctest
```

## Code Style

Formatting is enforced via `.clang-format` (LLVM-based, 4-space indent, 100 column limit, Attach brace style). Format with:

```bash
clang-format -i src/**/*.cpp src/**/*.hpp
```

The build uses `-Wall -Wextra -Wpedantic -Wconversion` — keep all warnings clean.

## Architecture

Xenon is a GTK 3 / GtkSourceView text editor in C++17. Source is organized into four static libraries that are linked into the final `xenon` binary:

- **`xenon_core`** (`src/core/`) — platform-agnostic text model: `Document` (buffer + encoding + line tracking), `EditHistory` (command-pattern undo/redo, 1000-command cap), `FileManager` (static I/O utils), `TextPosition`/`TextRange` (coordinates)
- **`xenon_ui`** (`src/ui/`) — all GTK widgets: `MainWindow` (1200×800 app window, menu bar, paned layout), `EditorWidget` (Gsv::View wrapper + per-editor Document), `SplitPaneContainer`, `FileExplorer` (lazy TreeView), `SearchReplaceDialog`, `QuickOpenDialog` (fuzzy), `TerminalWidget` (VTE)
- **`xenon_features`** (`src/features/`) — `SearchEngine` (static find/replace helpers supporting regex and case-sensitive modes)
- **`xenon_services`** (`src/services/`) — `SettingsManager` singleton (hand-written JSON, persists to `~/.config/xenon/settings.json`)

`src/main.cpp` initializes `Gtk::Application`, calls `Gsv::init()`, creates `MainWindow`, and runs the event loop.

### Window Layout

```
MainWindow (ApplicationWindow)
└── main_paned (HPaned)
    ├── FileExplorer (200px, left)
    └── content_vpaned (VPaned)
        ├── Gtk::Notebook (editor tabs)
        │   └── EditorWidget (one per tab)
        └── TerminalWidget (VTE, hidden by default)
```

### Optional Dependencies

The build detects and conditionally compiles optional features:

| Define | Feature |
|---|---|
| `HAVE_GTKSOURCEVIEW` | Syntax highlighting |
| `HAVE_GTKSOURCEVIEWMM` | C++ GtkSourceView bindings |
| `HAVE_VTE` | Integrated terminal |
| `HAVE_LIBGIT2` | Git integration (not yet implemented) |

GtkSourceView falls back from 3.0 → 4.0; GtkSourceViewmm tries 3.0 → 4.0 → 4; VTE tries 2.91 → 2.90.

## Key Keyboard Shortcuts (wired in `MainWindow`)

`Ctrl+N` new file, `Ctrl+O` open file, `Ctrl+S` save, `Ctrl+P` quick open, `Ctrl+H` search/replace.

--------

## Project scope and feature targets

Project Overview
This document serves as a comprehensive guide for developing a custom text editor tailored for professional use with primary languages including C++, C, Rust, and Go, while aiming to support all languages that Visual Studio Code (VSCode) accommodates. The editor will be implemented in C++ utilizing the GTK toolkit for the user interface. The goal is to replicate the extensive feature set of VSCode—encompassing syntax highlighting, code completion, debugging, version control integration, and more—while ensuring the editor's appearance and performance emulate Sublime Text: minimalist, responsive, lightweight, and highly efficient.
The editor should prioritize speed and low resource consumption, avoiding unnecessary bloat. It must handle large files seamlessly, provide a distraction-free coding environment, and incorporate modern UI elements for an intuitive user experience. Development will focus on modularity, extensibility, and adherence to high code quality standards to facilitate future maintenance and enhancements.
Key objectives:

Achieve feature parity with VSCode for core editing, language support, and productivity tools.
Mirror Sublime Text's visual simplicity and performance benchmarks (e.g., startup time under 100ms, smooth scrolling for files exceeding 1MB).
Use GTK for cross-platform compatibility (primarily Linux, with potential extensions to Windows and macOS via GTK ports).

Technology Stack

Core Language: C++ (standard C++17 or later for modern features like std::filesystem and lambdas).
UI Framework: GTK 4 (with GTKmm for C++ bindings to ensure object-oriented integration).
Syntax Highlighting and Language Features: Integrate GtkSourceView for syntax highlighting. For advanced language support (e.g., code completion, diagnostics), implement Language Server Protocol (LSP) client to interface with external language servers, mirroring VSCode's architecture.
Additional Libraries:
libgit2 for Git integration.
JSONCpp or nlohmann/json for handling configuration files and LSP communication.
Boost.Asio or similar for asynchronous operations (e.g., file I/O, network for extensions).
WebKitGTK for rendering web views if needed (e.g., for extension marketplace previews).

Build System: Use CMake for cross-platform builds, with dependencies managed via vcpkg or Conan.
Testing Framework: Google Test for unit tests; integrate Valgrind for memory leak detection.

Ensure all dependencies are statically linked where possible to minimize runtime requirements and enhance portability.
Target Features
The editor must implement every feature available in VSCode, adapted to a Sublime-like interface. Do not omit any feature; prioritize core editing tools first, then extensions and integrations. Features are categorized below for clarity.
Core Editing Features

Syntax Highlighting: Support for all VSCode-supported languages via GtkSourceView language definitions. Dynamically load themes (e.g., Monokai, Solarized) with customizable color schemes.
Code Completion (IntelliSense): LSP-based autocompletion with suggestions for variables, functions, methods, and snippets. Include parameter hints, signature help, and quick info hovers.
Code Folding: Collapse/expand regions like functions, classes, and comments.
Multiple Cursors and Selections: Allow simultaneous editing at multiple points, with commands for adding cursors (e.g., Ctrl+Click like Sublime).
Find and Replace: Global search across files, regex support, case sensitivity, whole word matching, and multi-line replace.
Snippets: User-defined and language-specific code snippets with tab stops and placeholders.
Auto-Indentation and Formatting: On-the-fly indentation, brace matching, and integration with formatters like clang-format via LSP.
Line and Block Commenting: Toggle comments for selected lines or blocks.
Undo/Redo Stack: Unlimited history with branching support.
Word Wrap and Line Endings: Configurable word wrap, detection/conversion of CRLF/LF.
Encoding Support: Handle UTF-8, UTF-16, ISO-8859, etc., with auto-detection.
Large File Handling: Efficiently edit files >1GB without performance degradation.
Minimap: A sidebar overview of the file structure, scrollable like Sublime.
Goto Definition/Symbol/Reference: LSP-powered navigation to definitions, implementations, and references.
Peek Definition: Inline preview of definitions without navigating away.
Rename Symbol: Refactor symbols across files.
Code Lens: Inline actions like "Run Tests" or "References" above symbols.
Breadcrumbs: Navigation bar showing file path and symbol hierarchy.
Diff Viewer: Side-by-side comparison of file versions or branches.
Markdown Preview: Built-in renderer for Markdown files with live updates.

Language Support

Primary: C++, C, Rust, Go (full LSP integration using clangd for C/C++, rust-analyzer for Rust, gopls for Go).
Extended: Support all VSCode languages (e.g., Python, JavaScript, Java, HTML/CSS, SQL, etc.) via configurable LSP servers. Allow users to install and configure additional servers.
Diagnostics and Linting: Display errors, warnings, and suggestions in the editor gutter and problems panel.
Hover Information: Detailed tooltips on symbols, including documentation and types.

Debugging

Integrated Debugger: Support for GDB (C/C++), LLDB, Delve (Go), and Rust-GDB. Features include breakpoints, watch variables, call stack, step in/out/over, conditional breakpoints, and variable inspection.
Debug Console: REPL-like console for evaluating expressions during debugging.
Multi-Session Debugging: Debug multiple processes simultaneously.

Version Control Integration

Git Support: Clone, commit, push/pull, branch management, staging, conflict resolution, blame, and history viewing using libgit2.
GitHub/GitLab Integration: Pull requests, issues, and code reviews (via web views if needed).
Other VCS: Extensible support for SVN, Mercurial via plugins.

Extensions and Customization

Extension System: Mimic VSCode's marketplace with a built-in store browser. Allow installation of extensions (written in Lua or JavaScript via embedded runtime like Duktape).
Themes and Icons: Customizable UI themes, icon sets, and keybindings.
Keybindings: Fully remappable, with presets for VSCode, Sublime, Vim, and Emacs modes.
Settings Sync: Cloud or file-based sync of user preferences.
Command Palette: Quick access to all commands (Ctrl+Shift+P like VSCode/Sublime).
Status Bar: Display file info, line/column, encoding, Git branch, and errors.
Sidebar and Panels: File explorer, search results, output, terminal, problems, and extensions panels.
Integrated Terminal: Embedded shell with multi-tab support.
Tasks and Build Systems: Configurable tasks for building, running, and testing (e.g., CMake integration for C++).
Emmet Support: Abbreviations for HTML/CSS.
Bracket Pair Colorization: Color-matched brackets for readability.
Zen Mode: Full-screen distraction-free editing.
Live Share: Real-time collaboration (implement via WebSockets if feasible).

Performance and Accessibility Features

High Performance: Optimize for sub-50ms response times on edits; use efficient data structures (e.g., rope for text buffers).
Accessibility: Screen reader support, high-contrast themes, keyboard navigation.
Multi-Window and Split Views: Multiple editor windows and split panes.
File Watcher: Auto-reload on external changes.
Search in Files: Multi-threaded search across workspace.
Workspace Management: Open folders as projects, with .workspace files for settings.

UI Design Guidelines
The UI must emulate Sublime Text's neat, clean, and modern aesthetic: minimal chrome, flat design, subtle animations, and high readability. Use GTK's CSS theming for customization.

Overall Layout: Tabbed interface for files, collapsible sidebar on the left (file tree), bottom panel for output/terminal. Avoid toolbars; rely on context menus and command palette.
Color Scheme: Default to a dark theme (e.g., grays/blues like Sublime's Monokai). Ensure high contrast ratios (>4.5:1) for text.
Fonts and Typography: Use system monospace fonts (e.g., JetBrains Mono) at 12-14pt. Anti-aliased rendering for smooth text.
Icons: Minimalist, flat icons (integrate Adwaita or custom SVG sets). No gradients; use subtle hover effects.
Animations: Smooth but subtle (e.g., 200ms fade for tooltips). Disable for performance mode.
Spacing and Padding: Generous padding (8-12px) for readability; compact mode for dense views.
Modern Elements: Rounded corners on tabs/panels (via GTK CSS: border-radius). Shadow effects for depth without clutter.
** Responsiveness**: Adaptive layout for window resizing; mobile-friendly if ported.
Customization: Allow CSS overrides for users to tweak UI (store in ~/.config/editor/theme.css).
GTK-Specific Instructions:
Use Gtk::Application and Gtk::Window as base.
Employ Gtk::Box for layouts, Gtk::Notebook for tabs, Gtk::TreeView for file explorer.
Style with Gtk::CssProvider: Apply classes like .editor-tab { background: #1e1e1e; color: #ffffff; }.
Ensure native look on GNOME/KDE by respecting system themes, but override for Sublime consistency.


Development Instructions
Coding Standards and Quality

Code Structure: Modular design with separate modules for core (editor buffer), UI (GTK wrappers), features (e.g., lsp_client.cpp), and utilities. Use namespaces (e.g., namespace Editor::Core).
Naming Conventions: CamelCase for classes/functions, snake_case for variables. Descriptive names (e.g., HandleCodeCompletion instead of Complete).
Error Handling: Use exceptions judiciously; prefer std::expected or result types for fallible operations. Log errors verbosely.
Memory Management: Smart pointers (std::unique_ptr, std::shared_ptr) exclusively; avoid raw pointers.
Concurrency: Use std::thread and mutexes for background tasks (e.g., LSP communication). Avoid UI blocking.
Documentation: Doxygen-style comments for all public APIs. Inline comments for complex logic.
Testing: Aim for >80% coverage. Write unit tests for core logic, integration tests for features.
Performance Profiling: Use perf or gprof; optimize hotspots (e.g., text rendering).
Security: Sanitize inputs (e.g., file paths); avoid eval-like in extensions.
Versioning: Semantic versioning; maintain CHANGELOG.md.
Build and Deployment: CI/CD with GitHub Actions; package as AppImage for distribution.
Code Reviews: Simulate self-reviews; ensure no dead code, consistent style (use clang-format).
Best Practices: Follow SOLID principles; keep functions <50 lines; refactor aggressively.
