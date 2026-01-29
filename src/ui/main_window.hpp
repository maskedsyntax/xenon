#pragma once

#include <gtkmm.h>
#include <memory>
#include "ui/split_pane_container.hpp"
#include "ui/quick_open_dialog.hpp"
#include "ui/search_replace_dialog.hpp"

namespace xenon::ui {

class MainWindow : public Gtk::ApplicationWindow {
public:
    explicit MainWindow(Glib::RefPtr<Gtk::Application> app);
    virtual ~MainWindow() = default;

protected:
    bool on_delete_event(GdkEventAny* /* any_event */) override;
    bool on_key_press_event(GdkEventKey* event) override;

private:
    Glib::RefPtr<Gtk::Application> app_;
    Gtk::Box main_box_{Gtk::ORIENTATION_VERTICAL};
    Gtk::Box content_box_{Gtk::ORIENTATION_HORIZONTAL};
    Gtk::MenuBar menubar_;
    Gtk::Notebook notebook_;
    Gtk::Statusbar statusbar_;
    std::unique_ptr<SearchReplaceDialog> search_dialog_;
    std::unique_ptr<QuickOpenDialog> quick_open_dialog_;
    std::vector<SplitPaneContainer*> split_panes_;
    std::string working_directory_;

    void setupMenuBar();
    void setupUI();
    void createNewTab();
    SplitPaneContainer* getCurrentSplitPane();
    EditorWidget* getActiveEditor();
    void onFileNew();
    void onFileOpen();
    void onFileSave();
    void onFileSaveAs();
    void onFileQuit();
    void onEditFind();
    void onEditFindReplace();
    void onQuickOpen();
    void onSplitHorizontal();
    void onSplitVertical();
};

} // namespace xenon::ui
