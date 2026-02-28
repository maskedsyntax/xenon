#include "ui/editor_widget.hpp"
#include "ui/settings_dialog.hpp"
#include "core/file_manager.hpp"
#include "features/search_engine.hpp"
#include "git/git_manager.hpp"
#include <algorithm>
#include <cctype>
#include <regex>
#include <filesystem>
#include <chrono>

#ifdef HAVE_GTKSOURCEVIEW
#include <gtksourceview/gtksource.h>
#endif

namespace fs = std::filesystem;

namespace xenon::ui {

int EditorWidget::extra_sel_counter_ = 0;

EditorWidget::EditorWidget()
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL),
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

    // Info bar for external-change notifications (hidden initially)
    setupInfoBar();

    // Scrolled window for the editor (horizontal minimap goes alongside)
    auto* editor_row = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    scroll_window_ = Gtk::manage(new Gtk::ScrolledWindow());
    scroll_window_->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    scroll_window_->add(*view);
    editor_row->pack_start(*scroll_window_, true, true);
    pack_start(*editor_row, true, true);

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
    setupHoverTooltip();

    // Multiple cursor key press handler (runs before default)
    source_view_->signal_key_press_event().connect(
        sigc::mem_fun(*this, &EditorWidget::onSourceViewKeyPress), false);

    show_all();
}

void EditorWidget::setupInfoBar() {
    auto* bar = Gtk::manage(new Gtk::InfoBar());
    info_bar_ = bar;
    bar->set_message_type(Gtk::MESSAGE_WARNING);
    bar->set_no_show_all(true);

    auto* label = Gtk::manage(new Gtk::Label("File changed externally."));
    info_label_ = label;
    auto* content_area = dynamic_cast<Gtk::Box*>(bar->get_content_area());
    if (content_area) content_area->pack_start(*label, false, false);

    bar->add_button("Reload", Gtk::RESPONSE_YES);
    bar->add_button("Dismiss", Gtk::RESPONSE_NO);
    bar->signal_response().connect([this](int resp) {
        info_bar_->hide();
        if (resp == Gtk::RESPONSE_YES && !file_path_.empty()) {
            try {
                std::string content = xenon::core::FileManager::readFile(file_path_);
                setContent(content);
            } catch (...) {}
        }
        external_change_pending_ = false;
    });

    pack_start(*bar, false, false);
    reorder_child(*bar, 0);  // Put it at the top
}

void EditorWidget::startWatchingFile() {
    stopWatchingFile();
    if (file_path_.empty()) return;

    auto gio_file = Gio::File::create_for_path(file_path_);
    try {
        file_monitor_ = gio_file->monitor_file(Gio::FILE_MONITOR_NONE);
        file_monitor_->signal_changed().connect(
            sigc::mem_fun(*this, &EditorWidget::onFileChanged));
    } catch (...) {}
}

void EditorWidget::stopWatchingFile() {
    if (file_monitor_) {
        file_monitor_->cancel();
        file_monitor_.reset();
    }
}

void EditorWidget::onFileChanged(const Glib::RefPtr<Gio::File>& /*file*/,
                                  const Glib::RefPtr<Gio::File>& /*other*/,
                                  Gio::FileMonitorEvent event) {
    if (event == Gio::FILE_MONITOR_EVENT_CHANGED ||
        event == Gio::FILE_MONITOR_EVENT_CREATED) {
        if (!external_change_pending_ && !isModified()) {
            external_change_pending_ = true;
            if (info_bar_) {
                info_bar_->show();
            }
        }
    }
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

// ---- Zoom ----

void EditorWidget::zoomIn() {
    font_size_pt_ = std::min(font_size_pt_ + 1, 36);
    Pango::FontDescription fd(base_font_family_ + " " + std::to_string(font_size_pt_));
    source_view_->override_font(fd);
}

void EditorWidget::zoomOut() {
    font_size_pt_ = std::max(font_size_pt_ - 1, 6);
    Pango::FontDescription fd(base_font_family_ + " " + std::to_string(font_size_pt_));
    source_view_->override_font(fd);
}

void EditorWidget::zoomReset() {
    font_size_pt_ = 11;
    Pango::FontDescription fd(base_font_family_ + " " + std::to_string(font_size_pt_));
    source_view_->override_font(fd);
}

// ---- Undo / Redo ----

void EditorWidget::undo() {
    if (source_buffer_->can_undo()) source_buffer_->undo();
}

void EditorWidget::redo() {
    if (source_buffer_->can_redo()) source_buffer_->redo();
}

bool EditorWidget::canUndo() const { return source_buffer_->can_undo(); }
bool EditorWidget::canRedo() const { return source_buffer_->can_redo(); }

// ---- Line / Block commenting ----

void EditorWidget::toggleLineComment() {
    auto lang = source_buffer_->get_language();
    std::string line_comment = "// ";
    if (lang) {
        std::string id = lang->get_id().raw();
        if (id == "python" || id == "ruby" || id == "sh" || id == "perl" ||
            id == "yaml" || id == "cmake")
            line_comment = "# ";
        else if (id == "lua")
            line_comment = "-- ";
        else if (id == "html" || id == "xml")
            line_comment = "";  // use block for markup
        else if (id == "sql")
            line_comment = "-- ";
    }
    if (line_comment.empty()) return;

    Gtk::TextIter sel_start, sel_end;
    source_buffer_->get_selection_bounds(sel_start, sel_end);
    int start_line = sel_start.get_line();
    int end_line   = sel_end.get_line();
    // If selection ends at column 0, don't include that line
    if (sel_end.get_line_offset() == 0 && end_line > start_line) {
        --end_line;
    }

    source_buffer_->begin_user_action();

    // Check if all lines are commented (to toggle off)
    bool all_commented = true;
    for (int ln = start_line; ln <= end_line && all_commented; ++ln) {
        auto iter = source_buffer_->get_iter_at_line(ln);
        // Skip whitespace
        while (!iter.ends_line() && (iter.get_char() == ' ' || iter.get_char() == '\t'))
            iter.forward_char();
        auto end_iter = iter;
        for (size_t i = 0; i < line_comment.size() && !end_iter.ends_line(); ++i)
            end_iter.forward_char();
        if (source_buffer_->get_text(iter, end_iter) != line_comment)
            all_commented = false;
    }

    for (int ln = start_line; ln <= end_line; ++ln) {
        auto iter = source_buffer_->get_iter_at_line(ln);
        if (all_commented) {
            // Remove comment prefix
            while (!iter.ends_line() && (iter.get_char() == ' ' || iter.get_char() == '\t'))
                iter.forward_char();
            auto end_iter = iter;
            for (size_t i = 0; i < line_comment.size() && !end_iter.ends_line(); ++i)
                end_iter.forward_char();
            if (source_buffer_->get_text(iter, end_iter) == line_comment)
                source_buffer_->erase(iter, end_iter);
        } else {
            // Insert comment prefix at start of line content
            while (!iter.ends_line() && (iter.get_char() == ' ' || iter.get_char() == '\t'))
                iter.forward_char();
            source_buffer_->insert(iter, line_comment);
        }
    }

    source_buffer_->end_user_action();
}

void EditorWidget::toggleBlockComment() {
    auto lang = source_buffer_->get_language();
    std::string block_start = "/* ";
    std::string block_end   = " */";
    if (lang) {
        std::string id = lang->get_id().raw();
        if (id == "html" || id == "xml") {
            block_start = "<!-- ";
            block_end   = " -->";
        } else if (id == "python") {
            block_start = '\"' + std::string("\"\"");
            block_end   = '\"' + std::string("\"\"");
        }
    }

    Gtk::TextIter sel_start, sel_end;
    source_buffer_->get_selection_bounds(sel_start, sel_end);
    std::string selected = source_buffer_->get_text(sel_start, sel_end);

    source_buffer_->begin_user_action();
    if (selected.substr(0, block_start.size()) == block_start &&
        selected.size() >= block_end.size() &&
        selected.substr(selected.size() - block_end.size()) == block_end) {
        // Remove block comment
        std::string uncommented = selected.substr(
            block_start.size(), selected.size() - block_start.size() - block_end.size());
        source_buffer_->erase(sel_start, sel_end);
        source_buffer_->insert(sel_start, uncommented);
    } else {
        source_buffer_->erase(sel_start, sel_end);
        source_buffer_->insert(sel_start, block_start + selected + block_end);
    }
    source_buffer_->end_user_action();
}

// ---- Settings ----

void EditorWidget::applySettings(const EditorSettings& s) {
    if (!source_view_) return;

    Pango::FontDescription font_desc(s.font_name);
    // Track the base font for zoom operations
    base_font_family_ = font_desc.get_family().raw();
    font_size_pt_ = static_cast<int>(font_desc.get_size() / Pango::SCALE);
    if (font_size_pt_ <= 0) font_size_pt_ = 11;
    source_view_->override_font(font_desc);
    source_view_->set_tab_width(static_cast<guint>(s.tab_width));
    source_view_->set_insert_spaces_instead_of_tabs(s.spaces_for_tabs);
    source_view_->set_show_line_numbers(s.show_line_numbers);
    source_view_->set_highlight_current_line(s.highlight_line);
    source_view_->set_auto_indent(s.auto_indent);
    source_view_->set_show_right_margin(s.show_right_margin);
    source_view_->set_right_margin_position(static_cast<guint>(s.right_margin_col));

    if (s.word_wrap) {
        source_view_->set_wrap_mode(Gtk::WRAP_WORD_CHAR);
    } else {
        source_view_->set_wrap_mode(Gtk::WRAP_NONE);
    }

    auto scheme_manager = Gsv::StyleSchemeManager::get_default();
    auto scheme = scheme_manager->get_scheme(s.color_scheme);
    if (!scheme) scheme = scheme_manager->get_scheme("classic");
    if (scheme && source_buffer_) {
        source_buffer_->set_style_scheme(scheme);
    }
}

// ---- Hover tooltip ----

void EditorWidget::setupHoverTooltip() {
    source_view_->set_has_tooltip(true);
    source_view_->signal_query_tooltip().connect(
        sigc::mem_fun(*this, &EditorWidget::onQueryTooltip), false);
}

bool EditorWidget::onQueryTooltip(int x, int y, bool /*keyboard_mode*/,
                                   const Glib::RefPtr<Gtk::Tooltip>& tooltip) {
    if (!lsp_client_ || !lsp_client_->isRunning() || file_path_.empty()) {
        return false;
    }

    // Convert window coords to buffer coords
    int buf_x = 0, buf_y = 0;
    source_view_->window_to_buffer_coords(Gtk::TEXT_WINDOW_TEXT, x, y, buf_x, buf_y);

    Gtk::TextIter iter;
    source_view_->get_iter_at_location(iter, buf_x, buf_y);
    int hover_line = iter.get_line();      // 0-based
    int hover_col  = iter.get_line_offset(); // 0-based

    // Store hover result in shared_ptr for async capture
    auto result = std::make_shared<std::string>();
    auto done   = std::make_shared<bool>(false);

    lsp_client_->requestHover("file://" + file_path_, hover_line, hover_col,
        [result, done](const std::string& content) {
            *result = content;
            *done = true;
        });

    // Busy-wait a short time (max 200ms) for the hover response
    // This is acceptable since GTK tooltip query is already on the main thread
    // and the LSP response comes from another thread quickly
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(200);
    while (!*done && std::chrono::steady_clock::now() < deadline) {
        Glib::MainContext::get_default()->iteration(false);
    }

    if (*done && !result->empty()) {
        // Trim and limit tooltip text
        std::string text = *result;
        if (text.size() > 512) text = text.substr(0, 509) + "...";
        tooltip->set_text(text);
        return true;
    }

    return false;
}

void EditorWidget::showHoverPopup(const std::string& /*content*/,
                                   int /*screen_x*/, int /*screen_y*/) {
    // Unused — using GTK tooltip mechanism instead
}

void EditorWidget::hideHoverPopup() {
    if (hover_popup_) {
        delete hover_popup_;
        hover_popup_ = nullptr;
    }
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
        // Add to the editor_row box (parent of scroll_window_)
        auto* parent = dynamic_cast<Gtk::Box*>(scroll_window_->get_parent());
        if (parent) {
            parent->pack_start(*minimap_widget_, false, false);
        }
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
    // Stop watching old file
    stopWatchingFile();

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

    // Start watching new file
    startWatchingFile();
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

// ---- Multiple cursors ----

void EditorWidget::clearExtraSelections() {
    for (auto& sel : extra_selections_) {
        source_buffer_->delete_mark(sel.start);
        source_buffer_->delete_mark(sel.end);
    }
    extra_selections_.clear();
}

void EditorWidget::selectNextOccurrence() {
    auto buf = source_buffer_;

    Gtk::TextIter sel_start, sel_end;
    bool has_selection = buf->get_selection_bounds(sel_start, sel_end);

    if (!has_selection) {
        // Select word under cursor
        auto iter = buf->get_iter_at_mark(buf->get_insert());
        if (!iter.inside_word() && !iter.ends_word()) return;
        auto word_start = iter, word_end = iter;
        if (!word_start.starts_word()) word_start.backward_word_start();
        if (!word_end.ends_word()) word_end.forward_word_end();
        buf->select_range(word_start, word_end);
        return;
    }

    std::string search_text = buf->get_text(sel_start, sel_end).raw();
    if (search_text.empty()) return;

    // Search forward from sel_end
    Gtk::TextIter match_start, match_end;
    bool found = sel_end.forward_search(
        search_text, Gtk::TEXT_SEARCH_TEXT_ONLY, match_start, match_end);

    if (!found) {
        // Wrap around from beginning
        auto buf_start = buf->begin();
        found = buf_start.forward_search(
            search_text, Gtk::TEXT_SEARCH_TEXT_ONLY, match_start, match_end);
    }

    if (!found) return;

    // Don't re-select the same range (wrap with single match in buffer)
    if (match_start.get_offset() == sel_start.get_offset()) return;

    // Save current selection as an extra cursor
    int id = ++extra_sel_counter_;
    std::string start_name = "xs_start_" + std::to_string(id);
    std::string end_name   = "xs_end_"   + std::to_string(id);

    ExtraSelection xs;
    xs.start = buf->create_mark(start_name, sel_start, true);  // left gravity
    xs.end   = buf->create_mark(end_name,   sel_end,   false); // right gravity
    extra_selections_.push_back(std::move(xs));

    // Move primary selection to next match
    buf->select_range(match_start, match_end);
    source_view_->scroll_to(match_start, 0.3);
}

bool EditorWidget::onSourceViewKeyPress(GdkEventKey* event) {
    // If no extra cursors, pass through immediately
    if (extra_selections_.empty()) return false;

    // Escape clears extra selections
    if (event->keyval == GDK_KEY_Escape) {
        clearExtraSelections();
        return true; // consume
    }

    // Ctrl+D: add next occurrence
    if (event->keyval == GDK_KEY_d && (event->state & GDK_CONTROL_MASK)) {
        selectNextOccurrence();
        return true;
    }

    // Determine what kind of edit this is
    bool is_backspace = (event->keyval == GDK_KEY_BackSpace);
    bool is_delete    = (event->keyval == GDK_KEY_Delete);
    gunichar uc       = gdk_keyval_to_unicode(event->keyval);
    bool is_printable = (uc != 0) && g_unichar_isprint(uc) &&
                        !(event->state & GDK_CONTROL_MASK) &&
                        !(event->state & GDK_MOD1_MASK);

    if (!is_backspace && !is_delete && !is_printable) {
        // Navigation / modifier keys: clear extra cursors and pass through
        clearExtraSelections();
        return false;
    }

    // Build UTF-8 string for printable char
    std::string insert_str;
    if (is_printable) {
        char utf8_buf[8] = {};
        g_unichar_to_utf8(uc, utf8_buf);
        insert_str = utf8_buf;
    }

    // Capture all ranges now (before any edits), sort high→low
    struct SelRange { int s, e; };
    std::vector<SelRange> ranges;
    ranges.reserve(extra_selections_.size());
    for (auto& sel : extra_selections_) {
        auto si = source_buffer_->get_iter_at_mark(sel.start);
        auto ei = source_buffer_->get_iter_at_mark(sel.end);
        ranges.push_back({si.get_offset(), ei.get_offset()});
    }
    std::sort(ranges.begin(), ranges.end(),
              [](const SelRange& a, const SelRange& b) { return a.s > b.s; });

    source_buffer_->begin_user_action();
    for (auto& r : ranges) {
        auto si = source_buffer_->get_iter_at_offset(r.s);
        auto ei = source_buffer_->get_iter_at_offset(r.e);

        if (is_backspace) {
            if (si != ei) {
                source_buffer_->erase(si, ei);
            } else if (!si.is_start()) {
                auto prev = si;
                prev.backward_char();
                source_buffer_->erase(prev, si);
            }
        } else if (is_delete) {
            if (si != ei) {
                source_buffer_->erase(si, ei);
            } else if (!si.is_end()) {
                auto next = si;
                next.forward_char();
                source_buffer_->erase(si, next);
            }
        } else {
            // Printable: erase selection (if any), then insert
            if (si != ei) source_buffer_->erase(si, ei);
            si = source_buffer_->get_iter_at_offset(r.s); // re-fetch after erase
            source_buffer_->insert(si, insert_str);
        }
    }
    source_buffer_->end_user_action();

    // Let the default handler process the primary cursor too
    return false;
}

// ---- Code folding ----

static Glib::RefPtr<Gtk::TextTag> getFoldTag(Glib::RefPtr<Gsv::Buffer>& buf) {
    auto tagTable = buf->get_tag_table();
    auto tag = tagTable->lookup("fold-hidden");
    if (!tag) {
        tag = buf->create_tag("fold-hidden");
        tag->property_invisible() = true;
    }
    return tag;
}

// Returns {foldStartLine, foldEndLine} exclusive, or {-1,-1} if nothing to fold.
static std::pair<int,int> findFoldRegion(Glib::RefPtr<Gsv::Buffer>& buf, int currentLine) {
    int lineCount = buf->get_line_count();

    auto lineStart = buf->get_iter_at_line(currentLine);
    auto lineEnd = lineStart;
    lineEnd.forward_to_line_end();
    std::string lineText = buf->get_text(lineStart, lineEnd).raw();

    // Determine indentation of current line
    int indent = 0;
    bool hasContent = false;
    for (char c : lineText) {
        if (c == ' ') indent++;
        else if (c == '\t') indent += 4;
        else { hasContent = true; break; }
    }
    if (!hasContent) return {-1, -1};

    int foldStartLine = currentLine + 1;
    if (foldStartLine >= lineCount) return {-1, -1};

    int foldEndLine = foldStartLine;
    for (int i = foldStartLine; i < lineCount; ++i) {
        auto iStart = buf->get_iter_at_line(i);
        auto iEnd = iStart;
        iEnd.forward_to_line_end();
        std::string iText = buf->get_text(iStart, iEnd).raw();

        // Blank line: tentatively include
        bool blank = true;
        for (char c : iText) {
            if (c != ' ' && c != '\t' && c != '\n' && c != '\r') { blank = false; break; }
        }
        if (blank) { foldEndLine = i + 1; continue; }

        int iIndent = 0;
        for (char c : iText) {
            if (c == ' ') iIndent++;
            else if (c == '\t') iIndent += 4;
            else break;
        }
        if (iIndent > indent) {
            foldEndLine = i + 1;
        } else {
            break;
        }
    }

    if (foldStartLine >= foldEndLine) return {-1, -1};
    return {foldStartLine, foldEndLine};
}

void EditorWidget::foldAtCursor() {
    auto insert = source_buffer_->get_iter_at_mark(source_buffer_->get_insert());
    auto [foldStart, foldEnd] = findFoldRegion(source_buffer_, insert.get_line());
    if (foldStart < 0) return;

    auto tag = getFoldTag(source_buffer_);
    auto siIter = source_buffer_->get_iter_at_line(foldStart);
    Gtk::TextIter eiIter;
    if (foldEnd >= source_buffer_->get_line_count()) {
        eiIter = source_buffer_->end();
    } else {
        eiIter = source_buffer_->get_iter_at_line(foldEnd);
    }

    // Check if already folded
    if (siIter.has_tag(tag)) return;
    source_buffer_->apply_tag(tag, siIter, eiIter);
}

void EditorWidget::unfoldAtCursor() {
    auto tag = getFoldTag(source_buffer_);
    auto insert = source_buffer_->get_iter_at_mark(source_buffer_->get_insert());
    int currentLine = insert.get_line();

    // Try folded region starting at the next line
    auto foldStart = source_buffer_->get_iter_at_line(
        std::min(currentLine + 1, source_buffer_->get_line_count() - 1));

    // If cursor itself is in a fold, adjust
    if (!foldStart.has_tag(tag)) {
        // Walk backwards to find the fold region
        foldStart = insert;
        while (foldStart.has_tag(tag) && !foldStart.is_start())
            foldStart.backward_line();
        foldStart.forward_line();
    }

    if (!foldStart.has_tag(tag)) return;

    auto foldEnd = foldStart;
    while (!foldEnd.is_end() && foldEnd.has_tag(tag))
        foldEnd.forward_line();

    source_buffer_->remove_tag(tag, foldStart, foldEnd);
}

void EditorWidget::unfoldAll() {
    auto tag = getFoldTag(source_buffer_);
    source_buffer_->remove_tag(tag, source_buffer_->begin(), source_buffer_->end());
}

} // namespace xenon::ui
