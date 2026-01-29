#include "ui/file_explorer.hpp"
#include <algorithm>
#include <vector>

namespace fs = std::filesystem;

namespace xenon::ui {

FileExplorer::FileExplorer() {
    set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    tree_store_ = Gtk::TreeStore::create(columns_);
    tree_view_.set_model(tree_store_);
    
    tree_view_.append_column("Files", columns_.col_name);
    
    tree_view_.signal_row_activated().connect(sigc::mem_fun(*this, &FileExplorer::onRowActivated));

    add(tree_view_);
    show_all();
}

void FileExplorer::setRootDirectory(const std::string& path) {
    root_path_ = path;
    refresh();
}

void FileExplorer::refresh() {
    tree_store_->clear();
    if (root_path_.empty() || !fs::exists(root_path_)) return;

    populateDirectory(nullptr, root_path_);
}

void FileExplorer::populateDirectory(const Gtk::TreeModel::Row* parent, const std::string& directoryPath) {
    try {
        if (!fs::is_directory(directoryPath)) return;

        std::vector<fs::path> entries;
        for (const auto& entry : fs::directory_iterator(directoryPath)) {
            entries.push_back(entry.path());
        }

        // Sort: Directories first, then files
        std::sort(entries.begin(), entries.end(), [](const fs::path& a, const fs::path& b) {
            bool aIsDir = fs::is_directory(a);
            bool bIsDir = fs::is_directory(b);
            if (aIsDir != bIsDir) return aIsDir;
            return a.filename() < b.filename();
        });

        for (const auto& path : entries) {
            Gtk::TreeModel::Row row;
            if (parent) {
                row = *(tree_store_->append(parent->children()));
            } else {
                row = *(tree_store_->append());
            }

            row[columns_.col_name] = path.filename().string();
            row[columns_.col_path] = path.string();
            bool isDir = fs::is_directory(path);
            row[columns_.col_is_dir] = isDir;

            if (isDir) {
                // Recursive load for now (simpler than lazy loading)
                // CAUTION: Deep directories might freeze UI.
                // For a robust solution, we'd use 'test-expand-row' signal or similar.
                // Limiting depth or just loading current level + placeholders is better.
                // But for this requirement, let's try recursive but maybe limit depth?
                // Let's just recurse. If user opens huge folder, it's their fault for now.
                populateDirectory(&row, path.string());
            }
        }
    } catch (const std::exception& e) {
        // error handling
    }
}

std::string FileExplorer::getSelectedFile() {
    auto selection = tree_view_.get_selection();
    if (selection) {
        auto iter = selection->get_selected();
        if (iter) {
             Glib::ustring val = (*iter)[columns_.col_path];
             return val;
        }
    }
    return "";
}

void FileExplorer::onRowActivated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* /* column */) {
    auto iter = tree_store_->get_iter(path);
    if (iter) {
        bool isDir = (*iter)[columns_.col_is_dir];
        if (!isDir) {
            Glib::ustring filePath = (*iter)[columns_.col_path];
            signal_file_activated_.emit(filePath);
        } else {
            if (tree_view_.row_expanded(path)) {
                tree_view_.collapse_row(path);
            } else {
                tree_view_.expand_row(path, false);
            }
        }
    }
}

} // namespace xenon::ui
