#pragma once

#include <gtkmm.h>
#include <memory>
#include "ui/split_pane_container.hpp"
#include "ui/quick_open_dialog.hpp"
#include "ui/search_replace_dialog.hpp"
#include "ui/file_explorer.hpp"
#include "ui/terminal_widget.hpp"

namespace xenon::ui {

class MainWindow : public Gtk::ApplicationWindow {
public:
    explicit MainWindow(Glib::RefPtr<Gtk::Application> app);
    virtual ~MainWindow() = default;

protected:
    bool on_delete_event(GdkEventAny* /* any_event */) override;

private:
    Glib::RefPtr<Gtk::Application> app_;
    Glib::RefPtr<Gtk::AccelGroup> accel_group_;
    Gtk::Box main_box_{Gtk::ORIENTATION_VERTICAL};
    Gtk::Paned main_paned_{Gtk::ORIENTATION_HORIZONTAL}; // Split Explorer and Content
    Gtk::Box content_box_{Gtk::ORIENTATION_HORIZONTAL};
    Gtk::Paned content_vpaned_{Gtk::ORIENTATION_VERTICAL};
    Gtk::MenuBar menubar_;
    Gtk::Notebook notebook_;
    Gtk::Statusbar statusbar_;
    std::unique_ptr<SearchReplaceDialog> search_dialog_;
    std::unique_ptr<QuickOpenDialog> quick_open_dialog_;
    std::unique_ptr<FileExplorer> file_explorer_;
    std::unique_ptr<TerminalWidget> terminal_widget_;
    std::string working_directory_;

    void setupMenuBar();
    void setupUI();
    void createNewTab();
    Gtk::Widget* createTabLabel(const std::string& title, Gtk::Widget* page);
    void closeTab(Gtk::Widget* page);
    SplitPaneContainer* getCurrentSplitPane();
    EditorWidget* getActiveEditor();
    void onFileNew();
    void onFileOpen();
    void onOpenFolder();
    void onFileSave();
    void onFileSaveAs();
    void onFileCloseTab(); // Close Tab
    void onFileQuit();
    void onEditFind();
    void onEditFindReplace();
    // Search dialog signal handlers
    void onFindNext();
    void onFindPrevious();
    void onReplace();
    void onReplaceAll();
    
    void onQuickOpen();
    void onSplitHorizontal();
    void onSplitVertical();
    void onSelectLanguage();
    void onToggleTerminal();
    
    // File Explorer handler
    void onExplorerFileActivated(const std::string& path);
};
} // namespace xenon::ui
