#pragma once

#include <gtkmm.h>
#include <memory>
#include <unordered_map>
#include "ui/split_pane_container.hpp"
#include "ui/quick_open_dialog.hpp"
#include "ui/search_replace_dialog.hpp"
#include "ui/file_explorer.hpp"
#include "ui/terminal_widget.hpp"
#include "ui/status_bar.hpp"
#include "ui/command_palette.hpp"
#include "ui/search_panel.hpp"
#include "ui/breadcrumb_bar.hpp"
#include "ui/problems_panel.hpp"
#include "ui/settings_dialog.hpp"
#include "lsp/lsp_client.hpp"
#include "git/git_manager.hpp"

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
    Gtk::Paned main_paned_{Gtk::ORIENTATION_HORIZONTAL};
    Gtk::Box content_box_{Gtk::ORIENTATION_HORIZONTAL};
    Gtk::Paned content_vpaned_{Gtk::ORIENTATION_VERTICAL};
    Gtk::MenuBar menubar_;
    Gtk::Notebook notebook_;
    StatusBar status_bar_;
    std::unique_ptr<SearchReplaceDialog> search_dialog_;
    std::unique_ptr<QuickOpenDialog> quick_open_dialog_;
    std::unique_ptr<FileExplorer> file_explorer_;
    std::unique_ptr<TerminalWidget> terminal_widget_;
    std::unique_ptr<CommandPalette> command_palette_;
    std::unique_ptr<SearchPanel> search_panel_;
    std::unique_ptr<ProblemsPanel> problems_panel_;
    std::unique_ptr<SettingsDialog> settings_dialog_;
    BreadcrumbBar breadcrumb_bar_;
    Gtk::Notebook sidebar_notebook_;
    EditorSettings current_settings_;
    std::string working_directory_;

    // LSP clients keyed by language server command (e.g. "clangd")
    std::unordered_map<std::string, std::shared_ptr<xenon::lsp::LspClient>> lsp_clients_;

    // Git
    std::shared_ptr<xenon::git::GitManager> git_manager_;

    std::shared_ptr<xenon::lsp::LspClient> getLspClientForEditor(EditorWidget* editor);
    void startLspServer(const std::string& command, const std::string& langId);

    void setupMenuBar();
    void setupUI();
    void setupCommands();
    void createNewTab();
    Gtk::Widget* createTabLabel(const std::string& title, Gtk::Widget* page);
    void closeTab(Gtk::Widget* page);
    SplitPaneContainer* getCurrentSplitPane();
    EditorWidget* getActiveEditor();
    void updateStatusBar();
    void connectEditorSignals(EditorWidget* editor);
    void updateTabLabel(const std::string& title);
    void markTabModified(bool modified);

    void onFileNew();
    void onFileOpen();
    void onOpenFolder();
    void onFileSave();
    void onFileSaveAs();
    void onFileCloseTab();
    void onFileQuit();
    void onEditFind();
    void onEditFindReplace();
    void onFindNext();
    void onFindPrevious();
    void onReplace();
    void onReplaceAll();
    void onQuickOpen();
    void onCommandPalette();
    void onSplitHorizontal();
    void onSplitVertical();
    void onSelectLanguage();
    void onToggleTerminal();
    void onToggleMinimap();
    void onToggleSidebar();
    void onGlobalSearch();
    void onPreferences();
    void applySettingsToAllEditors();
    void onExplorerFileActivated(const std::string& path);
    void onGotoDefinition();
    void onTriggerCompletion();
    void openFileAtLine(const std::string& path, int line, int col);
};

} // namespace xenon::ui
