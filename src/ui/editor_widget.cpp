#include "ui/editor_widget.hpp"
#include "core/file_manager.hpp"
#include "features/search_engine.hpp"
#include "git/git_manager.hpp"
#include <algorithm>
#include <cctype>
#include <regex>
#include <filesystem>

#ifdef HAVE_GTKSOURCEVIEW
#include <gtksourceview/gtksource.h>
#endif

namespace fs = std::filesystem;

namespace xenon::ui {

EditorWidget::EditorWidget()
    : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL),
      document_(std::make_unique<xenon::core::Document>()) {

    source_buffer_ = Gsv::Buffer::create();
    source_buffer_->set_highlight_matching_brackets(true);

    auto* view = Gtk::manage(new Gsv::View(source_buffer_));
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
    if (!scheme) scheme = scheme_manager->get_scheme("classic");
    if (scheme) source_buffer_->set_style_scheme(scheme);

    // Setup tags for diagnostics
    setupDiagnosticTags();

    // Scrolled window for the editor
    scroll_window_ = Gtk::manage(new Gtk::ScrolledWindow());
    scroll_window_->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    scroll_window_->add(*view);
    pack_start(*scroll_window_, true, true);

    // Connect cursor-moved signal
    source_buffer_->signal_mark_set().connect(
        sigc::mem_fun(*this, &EditorWidget::onCursorMoved));

    // Connect content changed signal (with debounced LSP sync)
    source_buffer_->signal_changed().connect([this]() {
        signal_content_changed_.emit();
        // Increment version and send didChange to LSP
        if (lsp_client_ && !file_path_.empty()) {
            ++doc_version_;
            lsp_client_->didChange("file://" + file_path_, getContent(), doc_version_);
        }
    });

    setupGitMarkAttributes();

    show_all();
}

void EditorWidget::setupDiagnosticTags() {
    error_tag_ = source_buffer_->create_tag("lsp_error");
    error_tag_->property_underline() = Pango::UNDERLINE_ERROR;
    error_tag_->property_underline_rgba() = Gdk::RGBA("red");

    warning_tag_ = source_buffer_->create_tag("lsp_warning");
    warning_tag_->property_underline() = Pango::UNDERLINE_ERROR;
    warning_tag_->property_underline_rgba() = Gdk::RGBA("orange");
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
    if (lang) return lang->get_name().raw();
    return "Plain Text";
}

std::string EditorWidget::getEncoding() const {
    if (document_) return document_->encoding();
    return "UTF-8";
}

std::string EditorWidget::getLineEnding() const {
    if (document_) {
        auto le = document_->lineEnding();
        if (le == "CRLF") return "CRLF";
        if (le == "CR") return "CR";
    }
    return "LF";
}

// ---- LSP integration ----

std::string EditorWidget::languageIdFromPath(const std::string& path) const {
    std::string ext = fs::path(path).extension().string();
    if (ext == ".cpp" || ext == ".cxx" || ext == ".cc" || ext == ".C") return "cpp";
    if (ext == ".c") return "c";
    if (ext == ".h" || ext == ".hpp" || ext == ".hxx") return "cpp";
    if (ext == ".rs") return "rust";
    if (ext == ".go") return "go";
    if (ext == ".py") return "python";
    if (ext == ".js") return "javascript";
    if (ext == ".ts") return "typescript";
    if (ext == ".java") return "java";
    if (ext == ".cs") return "csharp";
    if (ext == ".rb") return "ruby";
    if (ext == ".sh") return "shellscript";
    if (ext == ".json") return "json";
    if (ext == ".xml") return "xml";
    if (ext == ".html" || ext == ".htm") return "html";
    if (ext == ".css") return "css";
    if (ext == ".yaml" || ext == ".yml") return "yaml";
    if (ext == ".md") return "markdown";
    return "plaintext";
}

void EditorWidget::setLspClient(std::shared_ptr<xenon::lsp::LspClient> client) {
    lsp_client_ = std::move(client);

    if (lsp_client_) {
        // Register diagnostics callback — dispatched to GTK main loop
        lsp_client_->setDiagnosticsCallback(
            [this](const std::string& uri, std::vector<xenon::lsp::Diagnostic> diags) {
                // Only apply if this editor owns the file
                std::string my_uri = "file://" + file_path_;
                if (uri != my_uri) return;
                // Marshal to GTK main thread
                Glib::signal_idle().connect_once(
                    [this, d = std::move(diags)]() { applyDiagnostics(d); });
            });

        // Notify LSP about the current file
        if (!file_path_.empty()) {
            lsp_client_->didOpen("file://" + file_path_,
                                  languageIdFromPath(file_path_),
                                  getContent(), doc_version_);
        }
    }
}

void EditorWidget::applyDiagnostics(const std::vector<xenon::lsp::Diagnostic>& diags) {
    // Clear old diagnostic tags
    auto begin = source_buffer_->begin();
    auto end = source_buffer_->end();
    source_buffer_->remove_tag(error_tag_, begin, end);
    source_buffer_->remove_tag(warning_tag_, begin, end);

    for (const auto& d : diags) {
        auto start_iter = source_buffer_->get_iter_at_line_offset(
            std::max(0, d.line), std::max(0, d.col));
        auto end_iter = source_buffer_->get_iter_at_line_offset(
            std::max(0, d.endLine), std::max(0, d.endCol));

        if (start_iter == end_iter) {
            // Mark at least one character
            if (!end_iter.ends_line()) end_iter.forward_char();
        }

        auto& tag = (d.severity == 1) ? error_tag_ : warning_tag_;
        source_buffer_->apply_tag(tag, start_iter, end_iter);
    }
}

void EditorWidget::triggerCompletion() {
    if (!lsp_client_ || !lsp_client_->isRunning() || file_path_.empty()) return;

    auto [line, col] = getCursorPosition();
    // LSP uses 0-based line/col
    lsp_client_->requestCompletion(
        "file://" + file_path_, line - 1, col - 1,
        [this](std::vector<xenon::lsp::CompletionItem> items) {
            Glib::signal_idle().connect_once([this, items = std::move(items)]() {
                showCompletionPopup(items);
            });
        });
}

void EditorWidget::gotoDefinition() {
    if (!lsp_client_ || !lsp_client_->isRunning() || file_path_.empty()) return;

    auto [line, col] = getCursorPosition();
    lsp_client_->requestDefinition(
        "file://" + file_path_, line - 1, col - 1,
        [this](const std::string& uri, int def_line, int def_col) {
            // Strip file:// prefix
            std::string path = uri;
            if (path.substr(0, 7) == "file://") path = path.substr(7);
            Glib::signal_idle().connect_once([this, path, def_line, def_col]() {
                if (path == file_path_) {
                    // Jump within same file
                    auto iter = source_buffer_->get_iter_at_line_offset(def_line, def_col);
                    source_buffer_->place_cursor(iter);
                    source_view_->scroll_to(iter, 0.3);
                }
                // Cross-file navigation is handled by MainWindow via signal
            });
        });
}

void EditorWidget::showCompletionPopup(const std::vector<xenon::lsp::CompletionItem>& items) {
    if (items.empty()) return;

    hideCompletionPopup();

    // Get cursor screen position
    auto cursor_iter = source_buffer_->get_iter_at_mark(source_buffer_->get_insert());
    Gdk::Rectangle strong_rect, weak_rect;
    source_view_->get_iter_location(cursor_iter, strong_rect);

    // Convert buffer coords to window coords
    int win_x_buf = 0, win_y_buf = 0;
    source_view_->buffer_to_window_coords(
        Gtk::TEXT_WINDOW_WIDGET,
        strong_rect.get_x(), strong_rect.get_y(),
        win_x_buf, win_y_buf);

    // Get widget's absolute screen position
    int abs_x = 0, abs_y = 0;
    if (source_view_->get_window(Gtk::TEXT_WINDOW_WIDGET)) {
        source_view_->get_window(Gtk::TEXT_WINDOW_WIDGET)->get_origin(abs_x, abs_y);
    }
    (void)weak_rect;

    int popup_x = abs_x + win_x_buf;
    int popup_y = abs_y + win_y_buf + strong_rect.get_height() + 2;

    auto* popup = new Gtk::Window(Gtk::WINDOW_POPUP);
    popup->set_default_size(300, 200);
    popup->set_decorated(false);
    popup->move(popup_x, popup_y);

    auto* frame = Gtk::manage(new Gtk::Frame());
    auto* scroll = Gtk::manage(new Gtk::ScrolledWindow());
    auto* list = Gtk::manage(new Gtk::ListBox());

    scroll->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    scroll->set_min_content_height(200);
    scroll->add(*list);
    frame->add(*scroll);
    popup->add(*frame);

    std::string prefix = currentWordPrefix();

    for (const auto& item : items) {
        auto* row = Gtk::manage(new Gtk::ListBoxRow());
        auto* box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
        box->set_margin_start(8);
        box->set_margin_end(8);
        box->set_margin_top(4);
        box->set_margin_bottom(4);

        auto* label = Gtk::manage(new Gtk::Label(item.label));
        label->set_halign(Gtk::ALIGN_START);
        label->set_hexpand(true);
        box->pack_start(*label, true, true);

        if (!item.detail.empty()) {
            auto* detail = Gtk::manage(new Gtk::Label(item.detail));
            detail->get_style_context()->add_class("dim-label");
            detail->set_halign(Gtk::ALIGN_END);
            box->pack_end(*detail, false, false);
        }

        row->add(*box);
        list->append(*row);
    }

    list->signal_row_activated().connect([this, popup, &items, prefix](Gtk::ListBoxRow* row) {
        if (!row) return;
        int idx = row->get_index();
        if (idx >= 0 && idx < static_cast<int>(items.size())) {
            std::string insert_text = items[static_cast<size_t>(idx)].insertText;
            // Remove the prefix that's already typed
            if (insert_text.substr(0, prefix.size()) == prefix) {
                insert_text = insert_text.substr(prefix.size());
            }
            insertCompletion(insert_text);
        }
        delete popup;
        completion_popup_ = nullptr;
    });

    popup->show_all();
    completion_popup_ = popup;

    // Dismiss on focus loss
    popup->signal_focus_out_event().connect([this, popup](GdkEventFocus*) -> bool {
        delete popup;
        completion_popup_ = nullptr;
        return false;
    });

    // Dismiss on Escape
    popup->signal_key_press_event().connect([this, popup](GdkEventKey* e) -> bool {
        if (e->keyval == GDK_KEY_Escape) {
            delete popup;
            completion_popup_ = nullptr;
            return true;
        }
        return false;
    });
}

void EditorWidget::hideCompletionPopup() {
    if (completion_popup_) {
        delete completion_popup_;
        completion_popup_ = nullptr;
    }
}

void EditorWidget::insertCompletion(const std::string& text) {
    source_buffer_->begin_user_action();
    source_buffer_->insert_at_cursor(text);
    source_buffer_->end_user_action();
}

std::string EditorWidget::currentWordPrefix() const {
    auto cursor = source_buffer_->get_iter_at_mark(source_buffer_->get_insert());
    auto word_start = cursor;
    while (!word_start.starts_line()) {
        word_start.backward_char();
        gunichar c = word_start.get_char();
        if (!g_unichar_isalnum(c) && c != '_') {
            word_start.forward_char();
            break;
        }
    }
    return source_buffer_->get_text(word_start, cursor).raw();
}

// ---- Git diff gutter ----

void EditorWidget::setupGitMarkAttributes() {
    // Added lines: green left bar
    git_added_attrs_ = Gsv::MarkAttributes::create();
    Gdk::RGBA green;
    green.set_rgba(0.4, 0.8, 0.4, 1.0);
    git_added_attrs_->set_background(green);
    source_view_->set_mark_attributes("git-added", git_added_attrs_, 0);

    // Modified lines: orange left bar
    git_modified_attrs_ = Gsv::MarkAttributes::create();
    Gdk::RGBA orange;
    orange.set_rgba(0.9, 0.6, 0.1, 1.0);
    git_modified_attrs_->set_background(orange);
    source_view_->set_mark_attributes("git-modified", git_modified_attrs_, 0);

    // Deleted lines: red triangle marker
    git_deleted_attrs_ = Gsv::MarkAttributes::create();
    Gdk::RGBA red;
    red.set_rgba(0.9, 0.2, 0.2, 1.0);
    git_deleted_attrs_->set_background(red);
    source_view_->set_mark_attributes("git-deleted", git_deleted_attrs_, 0);

    source_view_->set_show_line_marks(true);
}

void EditorWidget::setGitManager(std::shared_ptr<xenon::git::GitManager> gm) {
    git_manager_ = std::move(gm);
    refreshGitDiff();
}

void EditorWidget::refreshGitDiff() {
    if (!git_manager_ || !git_manager_->isGitRepo() || file_path_.empty()) return;

    // Clear existing git marks
    auto begin = source_buffer_->begin();
    auto end = source_buffer_->end();
    source_buffer_->remove_source_marks(begin, end, "git-added");
    source_buffer_->remove_source_marks(begin, end, "git-modified");
    source_buffer_->remove_source_marks(begin, end, "git-deleted");

    auto hunks = git_manager_->getFileDiff(file_path_, getContent());
    for (const auto& hunk : hunks) {
        std::string cat;
        switch (hunk.type) {
            case xenon::git::DiffLineType::Added:    cat = "git-added";    break;
            case xenon::git::DiffLineType::Modified: cat = "git-modified"; break;
            case xenon::git::DiffLineType::Deleted:  cat = "git-deleted";  break;
            default: continue;
        }

        int max_line = source_buffer_->get_line_count() - 1;
        int line_start = std::min(hunk.start_line, max_line);
        int line_count = (hunk.type == xenon::git::DiffLineType::Deleted)
                            ? 1 : std::max(1, hunk.count);

        for (int i = 0; i < line_count; ++i) {
            int ln = std::min(line_start + i, max_line);
            auto iter = source_buffer_->get_iter_at_line(ln);
            source_buffer_->create_source_mark("", cat, iter);
        }
    }
}

void EditorWidget::toggleMinimap() {
#ifdef HAVE_GTKSOURCEVIEW
    if (!minimap_widget_) {
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

// ---- Search ----

void EditorWidget::findNext(const std::string& text, bool caseSensitive, bool regex) {
    if (text.empty()) return;

    std::string content = source_buffer_->get_text();
    Gtk::TextIter start, end;
    source_buffer_->get_selection_bounds(start, end);
    int offset = end.get_offset();

    auto result = xenon::features::SearchEngine::findNext(
        content, text, static_cast<size_t>(offset), caseSensitive, regex);

    if (result.offset == std::string::npos) {
        result = xenon::features::SearchEngine::findNext(content, text, 0, caseSensitive, regex);
    }

    if (result.offset != std::string::npos) {
        auto match_start = source_buffer_->get_iter_at_offset(static_cast<int>(result.offset));
        auto match_end = source_buffer_->get_iter_at_offset(
            static_cast<int>(result.offset + result.length));
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

    auto result = xenon::features::SearchEngine::findPrevious(
        content, text, static_cast<size_t>(offset), caseSensitive, regex);

    if (result.offset == std::string::npos) {
        result = xenon::features::SearchEngine::findPrevious(
            content, text, content.length(), caseSensitive, regex);
    }

    if (result.offset != std::string::npos) {
        auto match_start = source_buffer_->get_iter_at_offset(static_cast<int>(result.offset));
        auto match_end = source_buffer_->get_iter_at_offset(
            static_cast<int>(result.offset + result.length));
        source_buffer_->select_range(match_start, match_end);
        source_view_->scroll_to(match_start);
    }
}

void EditorWidget::replace(const std::string& text, const std::string& replacement,
                            bool caseSensitive, bool regex) {
    if (text.empty()) return;

    Gtk::TextIter start, end;
    source_buffer_->get_selection_bounds(start, end);
    std::string selection = source_buffer_->get_text(start, end);

    bool match = false;
    if (regex) {
        try {
            auto flags = std::regex_constants::ECMAScript;
            if (!caseSensitive) flags |= std::regex_constants::icase;
            std::regex re(text, flags);
            match = std::regex_match(selection, re);
        } catch (const std::regex_error&) {}
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

void EditorWidget::replaceAll(const std::string& text, const std::string& replacement,
                               bool caseSensitive, bool regex) {
    if (text.empty()) return;

    std::string content = source_buffer_->get_text();
    auto results = xenon::features::SearchEngine::findAll(content, text, caseSensitive, regex);
    if (results.empty()) return;

    source_buffer_->begin_user_action();
    for (auto it = results.rbegin(); it != results.rend(); ++it) {
        auto s = source_buffer_->get_iter_at_offset(static_cast<int>(it->offset));
        auto e = source_buffer_->get_iter_at_offset(static_cast<int>(it->offset + it->length));
        source_buffer_->erase(s, e);
        source_buffer_->insert(s, replacement);
    }
    source_buffer_->end_user_action();
}

// ---- Content / file management ----

void EditorWidget::setFilePath(const std::string& path) {
    // If LSP was tracking the old file, close it
    if (lsp_client_ && !file_path_.empty()) {
        lsp_client_->didClose("file://" + file_path_);
    }
    file_path_ = path;
    applyLanguageHighlighting();

    // Notify LSP of newly opened file
    if (lsp_client_ && !file_path_.empty()) {
        doc_version_ = 1;
        lsp_client_->didOpen("file://" + file_path_,
                              languageIdFromPath(file_path_),
                              getContent(), doc_version_);
    }
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
    if (file_path_.empty()) return;

    try {
        xenon::core::FileManager::writeFile(file_path_, getContent());
        source_buffer_->set_modified(false);
        if (lsp_client_) {
            lsp_client_->didSave("file://" + file_path_);
        }
    } catch (const std::exception& e) {
        Gtk::MessageDialog dialog(e.what(), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
        dialog.run();
    }
}

bool EditorWidget::isModified() const {
    return source_buffer_->get_modified();
}

void EditorWidget::onDocumentChanged() {}

void EditorWidget::applyLanguageHighlighting() {
    auto language_manager = Gsv::LanguageManager::get_default();
    Glib::RefPtr<Gsv::Language> language;

    if (!file_path_.empty()) {
        language = language_manager->guess_language(file_path_, "");
    }

    if (source_buffer_) {
        source_buffer_->set_highlight_syntax(true);
        if (language) source_buffer_->set_language(language);
    }
}

void EditorWidget::setLanguage(const std::string& langId) {
    auto language_manager = Gsv::LanguageManager::get_default();
    auto language = language_manager->get_language(langId);

    if (source_buffer_) {
        source_buffer_->set_highlight_syntax(true);
        if (language) source_buffer_->set_language(language);
    }
}

} // namespace xenon::ui
