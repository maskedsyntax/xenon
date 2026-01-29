# Xenon Text/Code Editor Requirements Document

## 1. Introduction

### 1.1 Purpose
This document outlines the functional and non-functional requirements for the development of Xenon, a text/code editor designed to provide a seamless and intuitive editing experience for developers. Xenon draws inspiration from Kate's simplicity and Visual Studio Code's (VSCode) user interface and keyboard shortcuts. The requirements specified herein are intended to guide coding agents in implementing the editor using C++ and the GTK 3 framework (with gtkmm-3.0 bindings).

### 1.2 Scope
Xenon will support core editing functionalities for multiple programming languages, including but not limited to C++, Python, and JavaScript. Key features include syntax highlighting, code completion, a built-in terminal, plugin system, file explorer, settings manager, search and replace, code formatter, code linter, quick file opening (Ctrl+P), tabs, split panes, and Git integration. Debugging capabilities are explicitly excluded from the current scope.

The editor will focus on productivity enhancements while maintaining a lightweight and extensible architecture.

### 1.3 Overview
Xenon aims to combine minimalistic design with modern usability features. It will be built as a desktop application leveraging GTK 3 for cross-platform compatibility (primarily targeting Linux, with support for Windows and macOS where feasible). The plugin system will allow for future expansions without altering the core codebase.

## 2. Functional Requirements

### 2.1 Core Editing Features
- **Syntax Highlighting**: Provide real-time syntax highlighting for supported languages, including C++, Python, JavaScript, and additional languages as plugins allow. Highlighting shall cover keywords, strings, comments, variables, and functions. Utilize GtkSourceView 3.0 language definitions and style schemes.
- **Code Completion**: Offer intelligent code completion suggestions based on context, such as auto-suggesting methods, variables, and keywords. Leverage GtkSourceView's completion framework (GtkSourceCompletion) with registered providers. This should integrate with language-specific logic or external tools (e.g., LSP clients in future extensions).
- **Search and Replace**: Implement a built-in search and replace functionality that supports regular expressions, case sensitivity, whole-word matching, and multi-file operations, using GtkSourceSearchSettings and related APIs.
- **Code Formatter**: Include a built-in formatter to automatically adjust code indentation, spacing, and style according to configurable rules (e.g., based on clang-format, black, or similar standards for supported languages).
- **Code Linter**: Integrate a linter to perform static code analysis, highlighting errors, warnings, and style issues in real-time or on demand (via gutter marks or diagnostics).

### 2.2 User Interface and Navigation
- **Tabs**: Support multiple open files in tabbed views (GtkNotebook), allowing users to switch, close, and reorder tabs easily.
- **Split Panes**: Enable splitting the editor window into multiple panes (horizontal and vertical) for simultaneous viewing and editing of files (using GtkPaned).
- **Quick File Opening (Ctrl+P)**: Implement a fuzzy search dialog triggered by Ctrl+P for rapid file navigation and opening within the workspace.
- **File Explorer**: Provide a built-in sidebar for file and directory management, including browsing, creating, deleting, renaming, and opening files/folders (using GtkTreeView with Gio model).
- **Settings Manager**: Offer a dedicated interface for customizing editor preferences, such as themes, keybindings, font sizes, and feature toggles. Settings should be stored in a configurable file (e.g., JSON) or GSettings where appropriate.

### 2.3 Integration and Extensions
- **Built-in Terminal**: Embed a terminal emulator within the editor for executing shell commands, supporting features like command history, auto-completion, and multi-terminal tabs. Utilize libvte (VTE 2.91 / GTK 3 compatible).
- **Plugin System**: Develop an extensible plugin architecture allowing third-party or user-developed plugins to add features, language support, or UI elements. Plugins should be loadable dynamically without restarting the editor (consider GObject-based modules or shared libraries with defined interfaces).
- **Git Integration**: Incorporate basic Git version control features, including status viewing, committing, pulling, pushing, branching, and diff viewing, accessible via UI elements or commands (recommended: libgit2).

### 2.4 Language and Framework Support
Support a wide range of programming languages and frameworks out-of-the-box via GtkSourceView language specifications, with extensibility via plugins. Initial support must include C++, Python, and JavaScript.

## 3. Non-Functional Requirements

### 3.1 Performance
- The editor should handle files up to 10 MB in size without significant lag in loading, editing, or highlighting.
- Startup time should be under 5 seconds on standard hardware.
- Features like code completion and linting should respond within 500 ms.

### 3.2 Usability
- Adopt VSCode-inspired keyboard shortcuts (e.g., Ctrl+S for save, Ctrl+F for search) while maintaining Kate-like simplicity in layout and operations.
- Ensure intuitive UI/UX with customizable themes (light/dark modes via GtkSourceStyleScheme) and accessible design principles (e.g., high contrast options).
- Provide tooltips, status bar indicators, and error messages for user guidance.

### 3.3 Reliability and Security
- Implement auto-save and recovery mechanisms for unsaved changes in case of crashes.
- Ensure secure handling of user data, avoiding unnecessary network access except for Git operations.
- The application should be stable, with minimal crashes, and include basic logging for debugging purposes.

### 3.4 Maintainability
- Codebase should follow modular design principles, with clear separation of concerns (e.g., UI, core logic, plugins).
- Include unit tests for core features and integration tests for plugins.

### 3.5 Compatibility
- Cross-platform support using GTK 3, ensuring consistent behavior across Linux (primary), Windows, and macOS (with noted limitations in native integration on non-Linux platforms).
- Minimum system requirements: 4 GB RAM, modern CPU, and GTK 3.22+.

## 4. Technical Constraints
- **Programming Language**: All core development must use C++ (standard C++17 or later).
- **Framework**: Utilize GTK 3 for GUI, widgets, and cross-platform functionality. Use gtkmm-3.0 for idiomatic C++ bindings. Leverage GtkSourceView 3.0 for the editor component.
- **Dependencies**: Minimize external dependencies. Required packages include:
  - libgtk-3-dev
  - libgtksourceview-3.0-dev
  - libvte-2.91-dev (for terminal)
  - libgtkmm-3.0-dev and libgtksourceviewmm-3.0-dev (C++ bindings)
  - For Git: libgit2-dev (recommended)
  - Avoid runtime dependencies that require installation beyond standard desktop environments.
- **Build System**: Use CMake for building and packaging. Use PkgConfig to locate GTK/gtkmm/GtkSourceView dependencies.

## 5. Assumptions and Dependencies
- **Assumptions**:
  - Users have basic familiarity with code editors.
  - The editor will be distributed as open-source under a permissive license (e.g., MIT).
  - Initial development focuses on desktop environments; mobile or web versions are out of scope.
  - Primary target is Linux/GNOME; Windows/macOS ports are secondary and may require additional theme/integration work.
- **Dependencies**:
  - GTK 3 (gtk+-3.0), gtkmm-3.0
  - GtkSourceView 3.0 (for syntax highlighting and completion framework)
  - VTE 2.91 (for embedded terminal)
  - External libraries only if essential (e.g., libgit2 for Git)
  - Git integration assumes users have Git installed on their system.

## 6. Exclusions
- Debugging tools (e.g., breakpoints, variable inspection) are not to be implemented in this version.
- Advanced collaboration features (e.g., real-time editing) are excluded.
- Support for proprietary languages or frameworks requiring licenses is not required.

This document serves as a comprehensive blueprint for implementing Xenon using GTK 3. Any deviations or additions should be documented and approved prior to development.

### CODING RULES:

You are writing production-grade code as a senior engineer.

The code must be secure, readable, and boring in the good way.

General rules:

1. No noob patterns
   - No global mutable state
   - No God objects or God files
   - No magic numbers or strings
   - No copy-paste logic
   - No commented-out code

2. Naming discipline
   - Variable names must encode intent, not type
   - Function names must describe behavior, not implementation
   - Avoid abbreviations unless universally obvious
   - Prefer explicit over clever

3. Structure and boundaries
   - Keep files small and focused
   - One responsibility per module
   - Dependencies flow inward, never outward
   - Core logic must be framework-agnostic

4. Error handling
   - Never swallow errors
   - Errors must be actionable and contextual
   - Distinguish between user errors and system errors
   - Fail fast for programmer errors
   - Validate inputs at boundaries only

5. Security by default
   - Treat all input as untrusted
   - Validate and sanitize inputs explicitly
   - Use allowlists, not blocklists
   - Avoid reflection and dynamic execution
   - Never trust client-side validation
   - Do not log secrets or tokens
   - Secrets must come from environment or config files
   - Use constant-time comparisons where applicable

6. Authentication and authorization
   - Explicit auth checks, never implicit
   - Authorization must be centralized
   - No role checks scattered across code
   - Deny by default

7. Data handling
   - Parameterized queries only
   - No raw SQL string concatenation
   - Enforce data constraints at both app and DB level
   - Use transactions for multi-step writes
   - Never assume DB writes succeed

8. Concurrency and state
   - Assume code may run concurrently
   - Avoid shared mutable state
   - Use locks only when unavoidable
   - Prefer immutability and message passing
   - Document concurrency assumptions

9. Logging and observability
   - Logs must be structured
   - Log intent, not noise
   - No debug logs in hot paths
   - Do not log PII by default

10. Configuration
    - Config must be explicit and typed
    - Fail on missing config
    - No hidden defaults for security-sensitive values

11. Testing discipline
    - Core logic must be testable without frameworks
    - Write tests for behavior, not implementation
    - Test failure paths
    - Avoid mocks unless necessary

12. Comments and documentation
    - No AI-like comments
    - No restating the obvious
    - Comment on "why", not "what"
    - Use README and ARCHITECTURE docs for big-picture decisions

13. Dependency hygiene
    - Minimize dependencies
    - Avoid abandoned libraries
    - Pin versions
    - Prefer standard library when possible

14. Performance mindset
    - Do not prematurely optimize
    - Avoid obvious inefficiencies
    - Measure before optimizing
    - Prefer simple algorithms unless proven insufficient

15. Code review mindset
    - Assume this code will be reviewed by someone smarter
    - Make intent obvious
    - Optimize for future maintainers

DO NOT COMMIT THIS FILE.
