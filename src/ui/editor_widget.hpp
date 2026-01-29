#pragma once

#include <gtkmm.h>
#include <memory>
#include <string>
#include "core/document.hpp"

#ifdef HAVE_GTKSOURCEVIEWMM
#include <gtksourceviewmm.h>
#endif

namespace xenon::ui {

class EditorWidget : public Gtk::ScrolledWindow {
public:
    EditorWidget();
    virtual ~EditorWidget() = default;

    void setContent(const std::string& content);
    std::string getContent() const;
    void saveFile();

    void setFilePath(const std::string& path) { file_path_ = path; }
    const std::string& getFilePath() const { return file_path_; }

    bool isModified() const;

private:
    std::unique_ptr<xenon::core::Document> document_;

#ifdef HAVE_GTKSOURCEVIEWMM
    Glib::RefPtr<gtksourceview::Buffer> source_buffer_;
    Glib::RefPtr<gtksourceview::View> source_view_;
#else
    Glib::RefPtr<Gtk::TextBuffer> text_buffer_;
    Gtk::TextView* text_view_;
#endif

    std::string file_path_;

    void onDocumentChanged();
    void applyLanguageHighlighting();
};

} // namespace xenon::ui
