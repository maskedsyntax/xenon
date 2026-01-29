#include "ui/quick_open_dialog.hpp"
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

namespace xenon::ui {

QuickOpenDialog::QuickOpenDialog(Gtk::Window& parent)
    : Gtk::Dialog("Open File", parent, true) {
    set_modal(true);
    set_default_size(600, 400);

    auto contentArea = get_content_area();
    contentArea->set_margin_top(12);
    contentArea->set_margin_bottom(12);
    contentArea->set_margin_start(12);
    contentArea->set_margin_end(12);
    contentArea->set_spacing(12);

    search_entry_.set_placeholder_text("Type to search files...");
    search_entry_.signal_search_changed().connect(sigc::mem_fun(*this, &QuickOpenDialog::onSearchChanged));
    contentArea->pack_start(search_entry_, false, false);

    scroll_.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    scroll_.add(file_list_);
    file_list_.signal_row_activated().connect(sigc::mem_fun(*this, &QuickOpenDialog::onRowActivated));
    contentArea->pack_start(scroll_, true, true);

    add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

    contentArea->show_all();
}

void QuickOpenDialog::setWorkingDirectory(const std::string& path) {
    if (working_dir_ != path) {
        working_dir_ = path;
        loadFiles();
    }
}

std::string QuickOpenDialog::getSelectedFile() const {
    return selected_file_;
}

void QuickOpenDialog::loadFiles() {
    all_files_.clear();

    if (working_dir_.empty() || !fs::is_directory(working_dir_)) {
        return;
    }
    
    // In a real app, do this in a thread
    try {
        for (const auto& entry : fs::recursive_directory_iterator(working_dir_)) {
            if (fs::is_regular_file(entry)) {
                FileEntry fe;
                fe.path = entry.path().string();
                fe.displayName = fs::relative(entry.path(), working_dir_).string();
                fe.isDirectory = false;
                all_files_.push_back(fe);
            }

            if (all_files_.size() > 10000) { // Increased limit
                break;
            }
        }
    } catch (const fs::filesystem_error&) {
    }

    std::sort(all_files_.begin(), all_files_.end(),
        [](const FileEntry& a, const FileEntry& b) {
            return a.displayName < b.displayName;
        });
}

void QuickOpenDialog::onSearchChanged() {
    std::string query = search_entry_.get_text();
    current_results_ = fuzzyFilter(query); // Cache current results
    updateFileList(current_results_);
}

void QuickOpenDialog::onRowActivated(Gtk::ListBoxRow* row) {
    if (row) {
        int index = row->get_index();
        // Use current_results_ instead of recalculating
        if (index >= 0 && index < static_cast<int>(current_results_.size())) {
            selected_file_ = current_results_[index].path;
            response(Gtk::RESPONSE_OK);
        }
    }
}

std::vector<FileEntry> QuickOpenDialog::fuzzyFilter(const std::string& query) {
    std::vector<std::pair<int, FileEntry>> scored;

    for (const auto& file : all_files_) {
        int score = calculateScore(file.displayName, query);
        if (score > 0) {
            scored.emplace_back(score, file);
        }
    }

    std::sort(scored.rbegin(), scored.rend(),
        [](const auto& a, const auto& b) { return a.first > b.first; });

    std::vector<FileEntry> results;
    for (const auto& [score, file] : scored) {
        results.push_back(file);
        if (results.size() >= 20) {
            break;
        }
    }

    return results;
}

int QuickOpenDialog::calculateScore(const std::string& filename, const std::string& query) {
    if (query.empty()) {
        return 1;
    }

    std::string lower_file;
    std::string lower_query;

    std::transform(filename.begin(), filename.end(),
                  std::back_inserter(lower_file),
                  [](unsigned char c) { return std::tolower(c); });
    std::transform(query.begin(), query.end(),
                  std::back_inserter(lower_query),
                  [](unsigned char c) { return std::tolower(c); });

    if (lower_file.find(lower_query) != std::string::npos) {
        return 100;
    }

    int score = 0;
    size_t queryPos = 0;

    for (size_t i = 0; i < lower_file.size() && queryPos < lower_query.size(); i++) {
        if (lower_file[i] == lower_query[queryPos]) {
            score += (i == 0 || lower_file[i - 1] == '/') ? 10 : 1;
            queryPos++;
        }
    }

    if (queryPos == lower_query.size()) {
        return score;
    }

    return 0;
}

void QuickOpenDialog::updateFileList(const std::vector<FileEntry>& files) {
    file_list_.foreach(
        [this](Gtk::Widget& w) { file_list_.remove(w); }
    );

    for (const auto& file : files) {
        auto label = Gtk::manage(new Gtk::Label(file.displayName));
        label->set_halign(Gtk::ALIGN_START);
        label->set_margin_start(12);
        label->set_margin_end(12);
        label->set_margin_top(6);
        label->set_margin_bottom(6);
        file_list_.append(*label);
    }

    file_list_.show_all();
}

} // namespace xenon::ui
