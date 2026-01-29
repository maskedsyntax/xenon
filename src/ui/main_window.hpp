#pragma once

#include <gtkmm.h>
#include <memory>
#include "ui/editor_widget.hpp"

namespace xenon::ui {

class MainWindow : public Gtk::ApplicationWindow {
public:
    explicit MainWindow(Glib::RefPtr<Gtk::Application> app);
    virtual ~MainWindow() = default;

protected:
    bool on_delete_event(GdkEventAny* any_event) override;
    bool on_key_press_event(GdkEventKey* event) override;

private:
    Glib::RefPtr<Gtk::Application> app_;
    Gtk::Box main_box_{Gtk::ORIENTATION_VERTICAL};
    Gtk::MenuBar menubar_;
    Gtk::Notebook notebook_;
    Gtk::Statusbar statusbar_;
    std::vector<std::unique_ptr<EditorWidget>> editors_;

    void setupMenuBar();
    void setupUI();
    void createNewTab();
    void onFileNew();
    void onFileOpen();
    void onFileSave();
    void onFileQuit();
};

} // namespace xenon::ui
