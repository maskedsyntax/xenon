#pragma once

#include <gtkmm.h>
#include <giomm.h>
#include <gtksourceviewmm.h>
#include <memory>
#include <string>
#include <vector>
#include "core/document.hpp"
#include "lsp/lsp_client.hpp"
#include "git/git_manager.hpp"
#include "ui/settings_dialog.hpp"

namespace xenon::ui {

class EditorWidget : public Gtk::Box {
public:
    EditorWidget();
    void setupInfoBar();
    virtual ~EditorWidget() = default;

    void setContent(const std::string& content);
    std::string getContent() const;
    void saveFile();

    void setFilePath(const std::string& path);
    const std::string& getFilePath() const { return file_path_; }

    void setLanguage(const std::string& lang);
    bool isModified() const;

    std::string getLanguageName() const;
    std::string getEncoding() const;
    std::string getLineEnding() const;
    std::pair<int, int> getCursorPosition() const;

    void findNext(const std::string& text, bool caseSensitive, bool regex);
    void findPrevious(const std::string& text, bool caseSensitive, bool regex);
    void replace(const std::string& text, const std::string& replacement, bool caseSensitive, bool regex);
    void replaceAll(const std::string& text, const std::string& replacement, bool caseSensitive, bool regex);

    void toggleMinimap();
    bool isMinimapVisible() const { return minimap_visible_; }

    // LSP
    void applySettings(const struct EditorSettings& s);
    void setLspClient(std::shared_ptr<xenon::lsp::LspClient> client);
    void triggerCompletion();
    void gotoDefinition();

    // Git diff gutter
    void setGitManager(std::shared_ptr<xenon::git::GitManager> gm);
    void refreshGitDiff();

    // File watcher
    void startWatchingFile();
    void stopWatchingFile();

    // LSP hover
    void setupHoverTooltip();

    // Signals
    sigc::signal<void, int, int>& signal_cursor_moved() { return signal_cursor_moved_; }
    sigc::signal<void>& signal_content_changed() { return signal_content_changed_; }

    Gsv::View* getSourceView() { return source_view_; }

private:
    std::unique_ptr<xenon::core::Document> document_;
    Glib::RefPtr<Gsv::Buffer> source_buffer_;
    Gsv::View* source_view_ = nullptr;
    Gtk::Widget* minimap_widget_ = nullptr;
    Gtk::ScrolledWindow* scroll_window_ = nullptr;
    std::string file_path_;
    bool minimap_visible_ = false;
    int doc_version_ = 1;

    // LSP
    std::shared_ptr<xenon::lsp::LspClient> lsp_client_;
    Gtk::Window* completion_popup_ = nullptr;
    Glib::RefPtr<Gtk::TextTag> error_tag_;
    Glib::RefPtr<Gtk::TextTag> warning_tag_;

    // Git
    std::shared_ptr<xenon::git::GitManager> git_manager_;

    // File watcher
    Glib::RefPtr<Gio::FileMonitor> file_monitor_;
    Gtk::InfoBar* info_bar_ = nullptr;
    Gtk::Label* info_label_ = nullptr;
    bool external_change_pending_ = false;

    void onFileChanged(const Glib::RefPtr<Gio::File>& file,
                       const Glib::RefPtr<Gio::File>& other,
                       Gio::FileMonitorEvent event);

    // Hover
    Gtk::Window* hover_popup_ = nullptr;
    sigc::connection hover_timeout_conn_;
    bool onQueryTooltip(int x, int y, bool keyboard_mode, const Glib::RefPtr<Gtk::Tooltip>& tooltip);
    void showHoverPopup(const std::string& content, int screen_x, int screen_y);
    void hideHoverPopup();
    Glib::RefPtr<Gsv::MarkAttributes> git_added_attrs_;
    Glib::RefPtr<Gsv::MarkAttributes> git_modified_attrs_;
    Glib::RefPtr<Gsv::MarkAttributes> git_deleted_attrs_;

    sigc::signal<void, int, int> signal_cursor_moved_;
    sigc::signal<void> signal_content_changed_;

    void onDocumentChanged();
    void applyLanguageHighlighting();
    void onCursorMoved(const Gtk::TextIter& iter, const Glib::RefPtr<Gtk::TextMark>& mark);

    // LSP helpers
    void setupDiagnosticTags();
    void setupGitMarkAttributes();
    void applyDiagnostics(const std::vector<xenon::lsp::Diagnostic>& diags);
    void showCompletionPopup(const std::vector<xenon::lsp::CompletionItem>& items);
    void hideCompletionPopup();
    void insertCompletion(const std::string& text);
    std::string currentWordPrefix() const;
    std::string languageIdFromPath(const std::string& path) const;
};

} // namespace xenon::ui
