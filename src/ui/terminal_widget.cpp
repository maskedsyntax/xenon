#include "ui/terminal_widget.hpp"
#include <cstdlib>

namespace xenon::ui {

TerminalWidget::TerminalWidget()
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL) {

#ifdef HAVE_VTE
    auto* terminal = vte_terminal_new();
    vte_terminal_ = VTE_TERMINAL(terminal);

    // Configure terminal appearance
    vte_terminal_set_size(vte_terminal_, 80, 12);
    vte_terminal_set_scroll_on_output(vte_terminal_, TRUE);
    vte_terminal_set_scroll_on_keystroke(vte_terminal_, TRUE);
    vte_terminal_set_scrollback_lines(vte_terminal_, 10000);

    // Set font
    PangoFontDescription* font = pango_font_description_from_string("Monospace 11");
    vte_terminal_set_font(vte_terminal_, font);
    pango_font_description_free(font);

    // Dark background
    GdkRGBA bg = {0.1, 0.1, 0.1, 1.0};
    GdkRGBA fg = {0.9, 0.9, 0.9, 1.0};
    vte_terminal_set_color_background(vte_terminal_, &bg);
    vte_terminal_set_color_foreground(vte_terminal_, &fg);

    // Wrap in gtkmm widget
    vte_widget_ = Glib::wrap(terminal);
    pack_start(*vte_widget_, true, true);

    spawnShell();
#else
    auto* label = Gtk::manage(new Gtk::Label("Terminal not available (VTE library not found)"));
    pack_start(*label, true, true);
#endif

    set_size_request(-1, 200);
}

void TerminalWidget::setWorkingDirectory(const std::string& path) {
    working_directory_ = path;
}

void TerminalWidget::toggle() {
    terminal_visible_ = !terminal_visible_;
    if (terminal_visible_) {
        set_no_show_all(false);
        show_all();
#ifdef HAVE_VTE
        if (vte_widget_) {
            vte_widget_->grab_focus();
        }
#endif
    } else {
        set_no_show_all(true);
        hide();
    }
}

#ifdef HAVE_VTE
void TerminalWidget::spawnShell() {
    const char* shell = std::getenv("SHELL");
    if (!shell) {
        shell = "/bin/bash";
    }

    const char* argv[] = {shell, nullptr};
    const char* working_dir = working_directory_.empty() ? nullptr : working_directory_.c_str();

    vte_terminal_spawn_async(
        vte_terminal_,
        VTE_PTY_DEFAULT,
        working_dir,
        const_cast<char**>(argv),
        nullptr,  // envv
        G_SPAWN_DEFAULT,
        nullptr,  // child_setup
        nullptr,  // child_setup_data
        nullptr,  // child_setup_data_destroy
        -1,       // timeout
        nullptr,  // cancellable
        nullptr,  // callback
        nullptr   // user_data
    );
}
#endif

} // namespace xenon::ui
