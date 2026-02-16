#pragma once

#include <gtkmm.h>
#include <string>

#ifdef HAVE_VTE
#include <vte/vte.h>
#endif

namespace xenon::ui {

class TerminalWidget : public Gtk::Box {
public:
    TerminalWidget();
    virtual ~TerminalWidget() = default;

    void setWorkingDirectory(const std::string& path);
    void toggle();
    bool isTerminalVisible() const { return terminal_visible_; }

private:
    bool terminal_visible_ = false;
    std::string working_directory_;

#ifdef HAVE_VTE
    Gtk::Widget* vte_widget_ = nullptr;
    VteTerminal* vte_terminal_ = nullptr;

    void spawnShell();
#endif
};

} // namespace xenon::ui
