#include "ui/editor_widget.hpp"
#include "core/file_manager.hpp"

namespace xenon::ui {

EditorWidget::EditorWidget()
    : document_(std::make_unique<xenon::core::Document>()) {
    set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    source_buffer_ = Gsv::Buffer::create();
    auto view = Gtk::manage(new Gsv::View(source_buffer_));
    source_view_ = view;

    view->set_show_line_numbers(true);
    view->set_tab_width(4);
    view->set_insert_spaces_instead_of_tabs(true);
    view->set_auto_indent(true);

    Pango::FontDescription font_desc("Monospace 11");
    view->override_font(font_desc);

    add(*view);
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

    source_buffer_->set_text(content);
    applyLanguageHighlighting();
}

std::string EditorWidget::getContent() const {
    return source_buffer_->get_text().raw();
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
    if (file_path_.empty()) {
        return;
    }

    try {
        std::string ext = xenon::core::FileManager::getFileExtension(file_path_);
        auto language_manager = Gsv::LanguageManager::get_default();

        if (!language_manager || !source_buffer_) {
            return;
        }

        Glib::RefPtr<Gsv::Language> language;

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
        } else if (ext == "rb") {
            language = language_manager->get_language("ruby");
        } else if (ext == "php") {
            language = language_manager->get_language("php");
        } else if (ext == "cs") {
            language = language_manager->get_language("csharp");
        } else if (ext == "json") {
            language = language_manager->get_language("json");
        } else if (ext == "xml" || ext == "html") {
            language = language_manager->get_language("xml");
        }

        if (language) {
            source_buffer_->set_language(language);
            source_buffer_->set_highlight_syntax(true);
        }
    } catch (const std::exception& /* e */) {
        // Silently ignore if language manager not ready
    }
}

void EditorWidget::setLanguage(const std::string& lang) {
    try {
        auto language_manager = Gsv::LanguageManager::get_default();
        if (language_manager) {
            auto language = language_manager->get_language(lang);
            if (language) {
                source_buffer_->set_language(language);
                source_buffer_->set_highlight_syntax(true);
            }
        }
    } catch (const std::exception& /* e */) {
        // Silently ignore if language manager not ready
    }
}

} // namespace xenon::ui
