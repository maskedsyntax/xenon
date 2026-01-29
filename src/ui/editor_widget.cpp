#include "ui/editor_widget.hpp"
#include "core/file_manager.hpp"

namespace xenon::ui {

EditorWidget::EditorWidget()
    : document_(std::make_unique<xenon::core::Document>()) {
    set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

#ifdef HAVE_GTKSOURCEVIEWMM
    source_buffer_ = gtksourceview::Buffer::create();
    source_view_ = gtksourceview::View::create(source_buffer_);

    source_view_->set_show_line_numbers(true);
    source_view_->set_tab_width(4);
    source_view_->set_insert_spaces_instead_of_tabs(true);
    source_view_->set_auto_indent(true);

    Pango::FontDescription font_desc("Monospace 11");
    source_view_->override_font(font_desc);

    add(*source_view_);
#else
    text_buffer_ = Gtk::TextBuffer::create();
    text_view_ = Gtk::manage(new Gtk::TextView(text_buffer_));

    text_view_->set_monospace(true);
    text_view_->set_wrap_mode(Gtk::WRAP_NONE);

    Pango::FontDescription font_desc("Monospace 11");
    text_view_->override_font(font_desc);

    add(*text_view_);
#endif

    show_all();
}

void EditorWidget::setContent(const std::string& content) {
    document_->clear();
    document_->insert(0, content);
    document_->resetModification();

    const std::string encoding = xenon::core::FileManager::detectEncoding(content);
    const std::string lineEnding = xenon::core::FileManager::detectLineEnding(content);

    document_->setEncoding(encoding);
    document_->setLineEnding(lineEnding);

#ifdef HAVE_GTKSOURCEVIEWMM
    source_buffer_->set_text(content);
    applyLanguageHighlighting();
#else
    text_buffer_->set_text(content);
#endif
}

std::string EditorWidget::getContent() const {
#ifdef HAVE_GTKSOURCEVIEWMM
    return source_buffer_->get_text().raw();
#else
    return text_buffer_->get_text().raw();
#endif
}

void EditorWidget::saveFile() {
    if (file_path_.empty()) {
        return;
    }

    try {
        xenon::core::FileManager::writeFile(file_path_, getContent());
        document_->resetModification();
    } catch (const std::exception& e) {
        Gtk::MessageDialog dialog(e.what(),
            false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
        dialog.run();
    }
}

bool EditorWidget::isModified() const {
    return document_->isModified();
}

void EditorWidget::onDocumentChanged() {
    // Update sync between GTK buffer and core Document
}

void EditorWidget::applyLanguageHighlighting() {
#ifdef HAVE_GTKSOURCEVIEWMM
    if (file_path_.empty()) {
        return;
    }

    std::string ext = xenon::core::FileManager::getFileExtension(file_path_);
    auto language_manager = gtksourceview::LanguageManager::get_default();

    Glib::RefPtr<gtksourceview::Language> language;

    if (ext == "cpp" || ext == "cc" || ext == "cxx" || ext == "h" || ext == "hpp") {
        language = language_manager->get_language("cpp");
    } else if (ext == "py") {
        language = language_manager->get_language("python");
    } else if (ext == "js") {
        language = language_manager->get_language("js");
    } else if (ext == "java") {
        language = language_manager->get_language("java");
    } else if (ext == "c") {
        language = language_manager->get_language("c");
    } else if (ext == "go") {
        language = language_manager->get_language("go");
    } else if (ext == "rs") {
        language = language_manager->get_language("rust");
    }

    if (language) {
        source_buffer_->set_language(language);
        source_buffer_->set_highlight_syntax(true);
    }
#endif
}

} // namespace xenon::ui
