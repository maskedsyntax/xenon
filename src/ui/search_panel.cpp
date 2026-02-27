#include "ui/search_panel.hpp"
#include <filesystem>
#include <fstream>
#include <regex>
#include <algorithm>

namespace fs = std::filesystem;

namespace xenon::ui {

// Directories and file extensions to skip during search
static const std::vector<std::string> SKIP_DIRS = {
    ".git", "build", "node_modules", ".cache", "__pycache__", "target", ".hg", ".svn"
};

static bool shouldSkipDir(const std::string& name) {
    for (const auto& d : SKIP_DIRS) {
        if (name == d) return true;
    }
    return false;
}

SearchPanel::SearchPanel()
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0) {

    // Header: search entry + options
    search_entry_.set_placeholder_text("Search files (Ctrl+Shift+F)");
    search_entry_.set_hexpand(true);
    case_check_.set_tooltip_text("Case sensitive");
    regex_check_.set_tooltip_text("Regular expression");

    header_box_.set_margin_start(4);
    header_box_.set_margin_end(4);
    header_box_.set_margin_top(4);
    header_box_.set_margin_bottom(4);
    header_box_.pack_start(search_entry_, true, true);
    header_box_.pack_start(case_check_, false, false);
    header_box_.pack_start(regex_check_, false, false);
    pack_start(header_box_, false, false);

    result_count_label_.set_text("");
    result_count_label_.set_halign(Gtk::ALIGN_START);
    result_count_label_.set_margin_start(6);
    result_count_label_.set_margin_bottom(2);
    result_count_label_.get_style_context()->add_class("dim-label");
    pack_start(result_count_label_, false, false);

    auto* sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
    pack_start(*sep, false, false);

    // Results tree
    tree_store_ = Gtk::TreeStore::create(cols_);
    tree_view_.set_model(tree_store_);
    tree_view_.set_headers_visible(false);
    tree_view_.set_activate_on_single_click(false);
    tree_view_.append_column("", cols_.display);
    tree_view_.get_column(0)->set_expand(true);

    // Style the text column
    auto* cell = dynamic_cast<Gtk::CellRendererText*>(
        tree_view_.get_column_cell_renderer(0));
    if (cell) {
        cell->property_ellipsize() = Pango::ELLIPSIZE_END;
    }

    scroll_.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    scroll_.add(tree_view_);
    pack_start(scroll_, true, true);

    search_entry_.signal_activate().connect(
        sigc::mem_fun(*this, &SearchPanel::onSearchActivated));
    search_entry_.signal_changed().connect([this]() {
        // Auto-search after short delay
        Glib::signal_timeout().connect_once([this]() {
            if (!search_entry_.get_text().empty()) {
                onSearchActivated();
            }
        }, 400);
    });

    tree_view_.signal_row_activated().connect(
        sigc::mem_fun(*this, &SearchPanel::onRowActivated));

    show_all();
}

SearchPanel::~SearchPanel() {
    stopSearch();
}

void SearchPanel::focusSearch() {
    search_entry_.grab_focus();
}

void SearchPanel::onSearchActivated() {
    std::string query = search_entry_.get_text().raw();
    if (query.empty()) {
        tree_store_->clear();
        result_count_label_.set_text("");
        return;
    }
    startSearch(query, case_check_.get_active(), regex_check_.get_active());
}

void SearchPanel::stopSearch() {
    cancel_search_ = true;
    if (search_thread_.joinable()) search_thread_.join();
    cancel_search_ = false;
    if (idle_conn_.connected()) idle_conn_.disconnect();
}

void SearchPanel::startSearch(const std::string& query, bool caseSensitive, bool useRegex) {
    stopSearch();

    tree_store_->clear();
    result_count_label_.set_text("Searching...");

    {
        std::lock_guard<std::mutex> lock(results_mutex_);
        pending_results_.clear();
    }

    // Start background search thread
    search_thread_ = std::thread([this, query, caseSensitive, useRegex]() {
        if (working_dir_.empty()) return;

        std::regex re;
        bool valid_regex = false;
        if (useRegex) {
            try {
                auto flags = std::regex_constants::ECMAScript;
                if (!caseSensitive) flags |= std::regex_constants::icase;
                re = std::regex(query, flags);
                valid_regex = true;
            } catch (...) {
                return;
            }
        }

        // Walk directory tree
        try {
            fs::recursive_directory_iterator dir_it(
                working_dir_,
                fs::directory_options::skip_permission_denied);
            fs::recursive_directory_iterator dir_end;

            for (; dir_it != dir_end; ++dir_it) {
                if (cancel_search_) break;

                const auto& entry = *dir_it;

                if (entry.is_directory()) {
                    if (shouldSkipDir(entry.path().filename().string())) {
                        dir_it.disable_recursion_pending();
                    }
                    continue;
                }

                if (!entry.is_regular_file()) continue;

                // Skip binary-looking files and large files
                auto size = entry.file_size();
                if (size > 10 * 1024 * 1024) continue;  // >10MB

                std::string filepath = entry.path().string();
                std::ifstream f(filepath, std::ios::binary);
                if (!f) continue;

                // Peek for null bytes (binary detection)
                char peek_buf[512];
                f.read(peek_buf, sizeof(peek_buf));
                auto read_count = f.gcount();
                bool binary = false;
                for (int i = 0; i < read_count; ++i) {
                    if (peek_buf[i] == '\0') { binary = true; break; }
                }
                if (binary) continue;

                // Reread from start
                f.seekg(0);
                std::string line;
                int line_num = 0;

                while (!cancel_search_ && std::getline(f, line)) {
                    ++line_num;
                    int col = 1;
                    bool found = false;

                    if (useRegex && valid_regex) {
                        std::smatch m;
                        if (std::regex_search(line, m, re)) {
                            found = true;
                            col = static_cast<int>(m.position()) + 1;
                        }
                    } else {
                        std::string hay = line, needle = query;
                        if (!caseSensitive) {
                            std::transform(hay.begin(), hay.end(), hay.begin(),
                                [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
                            std::transform(needle.begin(), needle.end(), needle.begin(),
                                [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
                        }
                        auto pos = hay.find(needle);
                        if (pos != std::string::npos) {
                            found = true;
                            col = static_cast<int>(pos) + 1;
                        }
                    }

                    if (found) {
                        SearchResult r;
                        r.file = filepath;
                        r.line = line_num;
                        r.col = col;
                        r.text = line;
                        // Trim leading whitespace
                        size_t first = r.text.find_first_not_of(" \t");
                        if (first != std::string::npos) r.text = r.text.substr(first);

                        std::lock_guard<std::mutex> lock(results_mutex_);
                        pending_results_.push_back(std::move(r));
                    }
                }
            }
        } catch (...) {}
    });

    // Flush results to UI periodically
    idle_conn_ = Glib::signal_timeout().connect(
        sigc::mem_fun(*this, &SearchPanel::onIdle), 100);
}

bool SearchPanel::onIdle() {
    flushResults();

    // Return true to keep repeating until search is done
    bool still_running = search_thread_.joinable() &&
        (cancel_search_ == false);

    if (!still_running) {
        flushResults();  // Final flush
        // Show final count
        int n = static_cast<int>(tree_store_->children().size());
        if (n == 0) {
            result_count_label_.set_text("No results");
        } else {
            result_count_label_.set_text(std::to_string(n) + " file(s) matched");
        }
        return false;  // Stop timer
    }
    return true;
}

void SearchPanel::flushResults() {
    std::vector<SearchResult> batch;
    {
        std::lock_guard<std::mutex> lock(results_mutex_);
        batch.swap(pending_results_);
    }

    for (auto& r : batch) {
        // Find or create file-level row
        Gtk::TreeModel::iterator file_row;
        bool found = false;
        for (auto& child : tree_store_->children()) {
            std::string cf = static_cast<std::string>(child[cols_.file]);
            if (cf == r.file) {
                file_row = child;
                found = true;
                break;
            }
        }

        if (!found) {
            file_row = tree_store_->append();
            fs::path p(r.file);
            (*file_row)[cols_.display] = p.filename().string() + "  " + p.parent_path().string();
            (*file_row)[cols_.file] = r.file;
            (*file_row)[cols_.line] = 0;
            (*file_row)[cols_.col] = 0;
        }

        // Add match row
        auto match_row = tree_store_->append(file_row->children());
        std::string display = "  " + std::to_string(r.line) + ":  " + r.text;
        (*match_row)[cols_.display] = display;
        (*match_row)[cols_.file] = r.file;
        (*match_row)[cols_.line] = r.line;
        (*match_row)[cols_.col] = r.col;
    }

    // Expand all file nodes with results
    tree_view_.expand_all();

    // Update count
    int n = static_cast<int>(tree_store_->children().size());
    if (n > 0) {
        result_count_label_.set_text(std::to_string(n) + " file(s)");
    }
}

void SearchPanel::onRowActivated(const Gtk::TreeModel::Path& path,
                                  Gtk::TreeViewColumn* /*col*/) {
    auto iter = tree_store_->get_iter(path);
    if (!iter) return;

    std::string file = (*iter)[cols_.file];
    int line = (*iter)[cols_.line];
    int col = (*iter)[cols_.col];

    if (!file.empty() && line > 0 && open_cb_) {
        open_cb_(file, line, col);
    }
}

} // namespace xenon::ui
