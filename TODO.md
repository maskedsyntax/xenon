# Xenon Editor: Roadmap & TODO (Qt Rewrite)

## 1. UI & UX Refinement (Current Focus)
- [x] **Qt 6 Rewrite:** Ported from GTK to Qt 6 for native macOS/Cross-platform look.
- [x] **Activity Bar & Sidebar:** Implemented Zed-like vertical navigation.
- [x] **Code Editor:** Basic syntax highlighting and line numbers.
- [x] **Terminal:** Integrated shell terminal using QProcess.
- [x] **Command Palette:** Modern floating palette (Cmd+Shift+P).
- [ ] **Fuzzy Search (File Open):** Improve QuickOpen with better fuzzy matching.
- [ ] **Tabs Management:** Add context menus to tabs (Close All, Close Others).
- [ ] **Native Mac Menus:** Further polish menu bar integration for macOS.

## 2. Core Features (Lacking)
- [ ] **Extensibility (Plugin System):** Integrate Lua scripting.
- [ ] **DAP Support:** Implement Debug Adapter Protocol.
- [ ] **Enhanced Git UI:** Sidebar for staging/committing.
- [ ] **LSP Polish:** Full support for Hover, Go-to-Definition, and Auto-complete.

## 3. Advanced
- [ ] **GPU Acceleration:** Explore Qt Quick/WGPU integration for the editor core.
- [ ] **AI Integration:** Native hooks for LLM assistants.
