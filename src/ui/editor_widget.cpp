#include "ui/editor_widget.hpp"
#include "core/file_manager.hpp"
#include "features/search_engine.hpp"
#include <algorithm>
#include <cctype>
#include <regex>

#ifdef HAVE_GTKSOURCEVIEW
#include <gtksourceview/gtksource.h>
#endif

namespace xenon::ui {

EditorWidget::EditorWidget()
    : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL),
      document_(std::make_unique<xenon::core::Document>()) {

    source_buffer_ = Gsv::Buffer::create();
    source_buffer_->set_highlight_matching_brackets(true);

    auto view = Gtk::manage(new Gsv::View(source_buffer_));
    source_view_ = view;

    view->set_show_line_numbers(true);
    view->set_tab_width(4);
    view->set_insert_spaces_instead_of_tabs(true);
    view->set_auto_indent(true);
    view->set_highlight_current_line(true);
    view->set_smart_home_end(Gsv::SMART_HOME_END_BEFORE);
    view->set_show_right_margin(true);
    view->set_right_margin_position(100);

    Pango::FontDescription font_desc("Monospace 11");
    view->override_font(font_desc);

    // Apply dark style scheme
    auto scheme_manager = Gsv::StyleSchemeManager::get_default();
    auto scheme = scheme_manager->get_scheme("oblivion");
    if (!scheme) {
        scheme = scheme_manager->get_scheme("classic");
    }
    if (scheme) {
        source_buffer_->set_style_scheme(scheme);
    }

    // Scrolled window for the main editor
    scroll_window_ = Gtk::manage(new Gtk::ScrolledWindow());
    scroll_window_->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    scroll_window_->add(*view);

    pack_start(*scroll_window_, true, true);

    // Connect cursor-moved signal
    source_buffer_->signal_mark_set().connect(
        sigc::mem_fun(*this, &EditorWidget::onCursorMoved));

    // Connect content changed signal
    source_buffer_->signal_changed().connect([this]() {
        signal_content_changed_.emit();
    });

    show_all();
}

void EditorWidget::onCursorMoved(const Gtk::TextIter& /*iter*/,
                                  const Glib::RefPtr<Gtk::TextMark>& mark) {
    if (mark->get_name() == "insert") {
        auto cursor = source_buffer_->get_iter_at_mark(source_buffer_->get_insert());
        int line = cursor.get_line() + 1;
        int col = cursor.get_line_offset() + 1;
        signal_cursor_moved_.emit(line, col);
    }
}

std::pair<int, int> EditorWidget::getCursorPosition() const {
    auto cursor = source_buffer_->get_iter_at_mark(source_buffer_->get_insert());
    return {cursor.get_line() + 1, cursor.get_line_offset() + 1};
}

std::string EditorWidget::getLanguageName() const {
    auto lang = source_buffer_->get_language();
    if (lang) {
        return lang->get_name().raw();
    }
    return "Plain Text";
}

std::string EditorWidget::getEncoding() const {
    if (document_) {
        return document_->encoding();
    }
    return "UTF-8";
}

std::string EditorWidget::getLineEnding() const {
    if (document_) {
        auto le = document_->lineEnding();
        if (le == "CRLF") return "CRLF";
        if (le == "CR") return "CR";
        return "LF";
    }
    return "LF";
}

void EditorWidget::toggleMinimap() {
#ifdef HAVE_GTKSOURCEVIEW
    if (!minimap_widget_) {
        // GtkSourceMap is available from GtkSourceView 3.18
        GtkWidget* map = gtk_source_map_new();
        gtk_source_map_set_view(GTK_SOURCE_MAP(map), GTK_SOURCE_VIEW(source_view_->gobj()));
        gtk_widget_set_size_request(map, 110, -1);

        minimap_widget_ = Glib::wrap(map);
        pack_start(*minimap_widget_, false, false);
        minimap_widget_->show();
        minimap_visible_ = true;
    } else if (minimap_visible_) {
        minimap_widget_->hide();
        minimap_visible_ = false;
    } else {
        minimap_widget_->show();
        minimap_visible_ = true;
    }
#endif
}

void EditorWidget::findNext(const std::string& text, bool caseSensitive, bool regex) {
    if (text.empty()) return;

    std::string content = source_buffer_->get_text();
    Gtk::TextIter start, end;
    source_buffer_->get_selection_bounds(start, end);
    int offset = end.get_offset();

    auto result = xenon::features::SearchEngine::findNext(content, text, static_cast<size_t>(offset), caseSensitive, regex);

    if (result.offset == std::string::npos) {
        // Wrap around
        result = xenon::features::SearchEngine::findNext(content, text, 0, caseSensitive, regex);
    }

    if (result.offset != std::string::npos) {
        auto match_start = source_buffer_->get_iter_at_offset(static_cast<int>(result.offset));
        auto match_end = source_buffer_->get_iter_at_offset(static_cast<int>(result.offset + result.length));
        source_buffer_->select_range(match_start, match_end);
        source_view_->scroll_to(match_start);
    }
}

void EditorWidget::findPrevious(const std::string& text, bool caseSensitive, bool regex) {
    if (text.empty()) return;

    std::string content = source_buffer_->get_text();
    Gtk::TextIter start, end;
    source_buffer_->get_selection_bounds(start, end);
    int offset = start.get_offset();

    auto result = xenon::features::SearchEngine::findPrevious(content, text, static_cast<size_t>(offset), caseSensitive, regex);

    if (result.offset == std::string::npos) {
        // Wrap around
        result = xenon::features::SearchEngine::findPrevious(content, text, content.length(), caseSensitive, regex);
    }

    if (result.offset != std::string::npos) {
        auto match_start = source_buffer_->get_iter_at_offset(static_cast<int>(result.offset));
        auto match_end = source_buffer_->get_iter_at_offset(static_cast<int>(result.offset + result.length));
        source_buffer_->select_range(match_start, match_end);
        source_view_->scroll_to(match_start);
    }
}

void EditorWidget::replace(const std::string& text, const std::string& replacement, bool caseSensitive, bool regex) {
    if (text.empty()) return;

    Gtk::TextIter start, end;
    source_buffer_->get_selection_bounds(start, end);
    std::string selection = source_buffer_->get_text(start, end);

    bool match = false;
    if (regex) {
        try {
            auto flags = std::regex_constants::ECMAScript;
            if (!caseSensitive) {
                flags |= std::regex_constants::icase;
            }
            std::regex re(text, flags);
            match = std::regex_match(selection, re);
        } catch (const std::regex_error&) {
            // Invalid regex, skip
        }
    } else if (caseSensitive) {
        match = (selection == text);
    } else {
        if (selection.length() == text.length()) {
            match = std::equal(selection.begin(), selection.end(), text.begin(),
                [](unsigned char a, unsigned char b) {
                    return std::tolower(a) == std::tolower(b);
                });
        }
    }

    if (match) {
        source_buffer_->begin_user_action();
        source_buffer_->erase(start, end);
        source_buffer_->insert(start, replacement);
        source_buffer_->end_user_action();
        findNext(text, caseSensitive, regex);
        return;
    }

    findNext(text, caseSensitive, regex);
}

void EditorWidget::replaceAll(const std::string& text, const std::string& replacement, bool caseSensitive, bool regex) {
    if (text.empty()) return;

    std::string content = source_buffer_->get_text();
    auto results = xenon::features::SearchEngine::findAll(content, text, caseSensitive, regex);

    if (results.empty()) return;

    source_buffer_->begin_user_action();

    // Iterate backwards to keep offsets valid
    for (auto it = results.rbegin(); it != results.rend(); ++it) {
        auto s = source_buffer_->get_iter_at_offset(static_cast<int>(it->offset));
        auto e = source_buffer_->get_iter_at_offset(static_cast<int>(it->offset + it->length));
        source_buffer_->erase(s, e);
        source_buffer_->insert(s, replacement);
    }

    source_buffer_->end_user_action();
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
    return source_buffer_->get_modified();
}

void EditorWidget::onDocumentChanged() {
    // Sync between GTK buffer and core Document
}

void EditorWidget::applyLanguageHighlighting() {
    auto language_manager = Gsv::LanguageManager::get_default();
    Glib::RefPtr<Gsv::Language> language;

    if (!file_path_.empty()) {
        language = language_manager->guess_language(file_path_, "");
    }

    if (source_buffer_) {
        source_buffer_->set_highlight_syntax(true);
        if (language) {
            source_buffer_->set_language(language);
        }
    }
}

void EditorWidget::setLanguage(const std::string& langId) {
    auto language_manager = Gsv::LanguageManager::get_default();
    auto language = language_manager->get_language(langId);

    if (source_buffer_) {
        source_buffer_->set_highlight_syntax(true);
        if (language) {
            source_buffer_->set_language(language);
        }
    }
}

} // namespace xenon::ui
