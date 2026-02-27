#pragma once

#include <gtkmm.h>
#include <gtksourceviewmm.h>
#include <memory>
#include <string>
#include "core/document.hpp"

namespace xenon::ui {

class EditorWidget : public Gtk::Box {
public:
    EditorWidget();
    virtual ~EditorWidget() = default;

    void setContent(const std::string& content);
    std::string getContent() const;
    void saveFile();

    void setFilePath(const std::string& path) {
        file_path_ = path;
        applyLanguageHighlighting();
    }
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
    Gtk::Paned* editor_paned_ = nullptr;
    std::string file_path_;
    bool minimap_visible_ = false;

    sigc::signal<void, int, int> signal_cursor_moved_;
    sigc::signal<void> signal_content_changed_;

    void onDocumentChanged();
    void applyLanguageHighlighting();
    void onCursorMoved(const Gtk::TextIter& iter, const Glib::RefPtr<Gtk::TextMark>& mark);
};

} // namespace xenon::ui
