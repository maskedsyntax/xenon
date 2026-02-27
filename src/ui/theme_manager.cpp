#include "ui/theme_manager.hpp"

namespace xenon::ui {

const char* ThemeManager::DARK_THEME_CSS = R"CSS(
/* ========== Xenon Dark Theme ========== */

/* Base window */
window, .background {
    background-color: #1e1e1e;
    color: #d4d4d4;
}

/* Menu bar */
menubar {
    background-color: #2d2d2d;
    color: #d4d4d4;
    border-bottom: 1px solid #3c3c3c;
    padding: 1px 0;
}

menubar > menuitem {
    padding: 4px 10px;
    color: #d4d4d4;
}

menubar > menuitem:hover {
    background-color: #094771;
    color: #ffffff;
}

/* Dropdown menus */
menu {
    background-color: #252526;
    color: #d4d4d4;
    border: 1px solid #454545;
    padding: 4px 0;
}

menu menuitem {
    padding: 6px 20px;
    color: #d4d4d4;
}

menu menuitem:hover,
menu menuitem:selected {
    background-color: #094771;
    color: #ffffff;
}

menu separator {
    background-color: #454545;
    margin: 4px 0;
}

/* Notebook (tabs) */
notebook {
    background-color: #1e1e1e;
}

notebook > header {
    background-color: #252526;
    border-bottom: 1px solid #3c3c3c;
    padding: 0;
}

notebook > header > tabs > tab {
    background-color: #2d2d2d;
    color: #969696;
    border: none;
    border-right: 1px solid #1e1e1e;
    padding: 6px 12px;
    min-width: 80px;
}

notebook > header > tabs > tab:checked {
    background-color: #1e1e1e;
    color: #ffffff;
    border-top: 2px solid #007acc;
}

notebook > header > tabs > tab:hover:not(:checked) {
    background-color: #2a2d2e;
    color: #cccccc;
}

/* Tab close button */
notebook > header > tabs > tab button {
    background: transparent;
    border: none;
    padding: 2px;
    color: #969696;
    min-width: 16px;
    min-height: 16px;
}

notebook > header > tabs > tab button:hover {
    background-color: #e81123;
    color: #ffffff;
    border-radius: 3px;
}

/* Source view (editor) */
textview,
textview.view {
    background-color: #1e1e1e;
    color: #d4d4d4;
    caret-color: #aeafad;
}

textview.view > .view {
    background-color: #1e1e1e;
}

/* Line numbers gutter */
textview.view .left-margin {
    background-color: #1e1e1e;
    color: #858585;
}

/* Scrollbars */
scrollbar {
    background-color: #1e1e1e;
    border: none;
}

scrollbar trough {
    background-color: #1e1e1e;
    min-width: 8px;
    min-height: 8px;
}

scrollbar slider {
    background-color: #424242;
    border-radius: 4px;
    min-width: 8px;
    min-height: 8px;
    border: 2px solid transparent;
    background-clip: padding-box;
}

scrollbar slider:hover {
    background-color: #686868;
}

scrollbar slider:active {
    background-color: #929292;
}

/* Status bar */
.xenon-statusbar {
    background-color: #007acc;
    color: #ffffff;
    min-height: 24px;
    border-top: none;
}

.statusbar-item {
    font-size: 12px;
    color: #ffffff;
}

.statusbar-item:hover {
    background-color: rgba(255,255,255,0.15);
}

.statusbar-message {
    font-size: 12px;
    color: #ffffff;
}

/* Sidebar separator */
.xenon-statusbar separator {
    background-color: rgba(255,255,255,0.2);
    min-width: 1px;
    margin: 4px 0;
}

/* File explorer (TreeView) */
treeview {
    background-color: #252526;
    color: #d4d4d4;
    border-right: 1px solid #3c3c3c;
}

treeview:selected,
treeview row:selected {
    background-color: #094771;
    color: #ffffff;
}

treeview row:hover {
    background-color: #2a2d2e;
}

treeview header button {
    background-color: #252526;
    color: #d4d4d4;
    border: none;
}

/* Paned separators */
paned > separator {
    background-color: #3c3c3c;
    min-width: 1px;
    min-height: 1px;
}

/* Dialog boxes */
dialog {
    background-color: #252526;
    color: #d4d4d4;
}

dialog .dialog-action-area {
    background-color: #252526;
    border-top: 1px solid #3c3c3c;
    padding: 8px;
}

/* Buttons */
button {
    background-color: #3c3c3c;
    color: #d4d4d4;
    border: 1px solid #555555;
    border-radius: 3px;
    padding: 5px 14px;
    min-height: 26px;
}

button:hover {
    background-color: #4a4a4a;
    border-color: #666666;
}

button:active {
    background-color: #007acc;
    border-color: #007acc;
    color: #ffffff;
}

button.suggested-action {
    background-color: #0e639c;
    color: #ffffff;
    border-color: #0e639c;
}

button.suggested-action:hover {
    background-color: #1177bb;
}

/* Text entries */
entry {
    background-color: #3c3c3c;
    color: #d4d4d4;
    border: 1px solid #555555;
    border-radius: 3px;
    padding: 4px 8px;
    caret-color: #aeafad;
}

entry:focus {
    border-color: #007acc;
    outline: none;
}

/* Checkboxes */
checkbutton {
    color: #d4d4d4;
}

check {
    background-color: #3c3c3c;
    border: 1px solid #555555;
    border-radius: 3px;
    min-width: 16px;
    min-height: 16px;
}

check:checked {
    background-color: #007acc;
    border-color: #007acc;
}

/* List boxes (for command palette, quick open, etc.) */
listbox {
    background-color: #252526;
    color: #d4d4d4;
}

listbox row {
    padding: 6px 12px;
    border-bottom: 1px solid #2d2d2d;
}

listbox row:selected {
    background-color: #094771;
    color: #ffffff;
}

listbox row:hover {
    background-color: #2a2d2e;
}

/* Scrolled window */
scrolledwindow {
    background-color: #1e1e1e;
}

/* Terminal widget container */
.xenon-terminal {
    background-color: #1e1e1e;
    border-top: 1px solid #3c3c3c;
}

/* Paned handle */
paned > separator.wide {
    background-color: #3c3c3c;
}

/* GtkSourceMap (minimap) */
GtkSourceMap,
gsvsourcemap {
    background-color: #1a1a1a;
    border-left: 1px solid #3c3c3c;
    font-size: 2px;
}
)CSS";

ThemeManager& ThemeManager::instance() {
    static ThemeManager inst;
    return inst;
}

void ThemeManager::applyDarkTheme(Glib::RefPtr<Gdk::Screen> screen) {
    if (!css_provider_) {
        css_provider_ = Gtk::CssProvider::create();
        try {
            css_provider_->load_from_data(DARK_THEME_CSS);
        } catch (const Glib::Error& e) {
            // CSS parse error — continue without custom theme
            return;
        }
    }

    Gtk::StyleContext::add_provider_for_screen(
        screen, css_provider_,
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

} // namespace xenon::ui
