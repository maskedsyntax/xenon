#pragma once

#include <gtkmm.h>
#include <gtksourceviewmm.h>
#include <memory>
#include <string>
#include "core/document.hpp"

namespace xenon::ui {

class EditorWidget : public Gtk::ScrolledWindow {
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

    void findNext(const std::string& text, bool caseSensitive, bool regex);
    void findPrevious(const std::string& text, bool caseSensitive, bool regex);
    void replace(const std::string& text, const std::string& replacement, bool caseSensitive, bool regex);
    void replaceAll(const std::string& text, const std::string& replacement, bool caseSensitive, bool regex);

private:
    std::unique_ptr<xenon::core::Document> document_;
    Glib::RefPtr<Gsv::Buffer> source_buffer_;
    Gsv::View* source_view_;
    std::string file_path_;

    void onDocumentChanged();
    void applyLanguageHighlighting();
};

} // namespace xenon::ui
