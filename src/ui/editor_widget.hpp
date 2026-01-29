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

    void setFilePath(const std::string& path) { file_path_ = path; }
    const std::string& getFilePath() const { return file_path_; }

    bool isModified() const;

private:
    std::unique_ptr<xenon::core::Document> document_;
    Glib::RefPtr<Gsv::Buffer> source_buffer_;
    Gsv::View* source_view_;
    std::string file_path_;

    void onDocumentChanged();
    void applyLanguageHighlighting();
};

} // namespace xenon::ui
