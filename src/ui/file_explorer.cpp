#include "ui/file_explorer.hpp"
#include <algorithm>

namespace fs = std::filesystem;

namespace xenon::ui {

FileExplorer::FileExplorer() {
    set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    // Setup columns model
    auto model_columns = new Gtk::TreeModel::ColumnRecord();
    auto col_name = new Gtk::TreeModelColumn<Glib::ustring>();
    model_columns->add(*col_name);

    tree_store_ = Gtk::TreeStore::create(*model_columns);

    tree_view_.set_model(tree_store_);
    tree_view_.append_column("Files", *col_name);
    tree_view_.signal_row_activated().connect(sigc::mem_fun(*this, &FileExplorer::onRowActivated));

    auto selection = tree_view_.get_selection();
    if (selection) {
        selection->signal_changed().connect(sigc::mem_fun(*this, &FileExplorer::onSelectionChanged));
    }

    add(tree_view_);
    show_all();
}

void FileExplorer::setRootDirectory(const std::string& path) {
    if (!fs::is_directory(path)) {
        return;
    }

    tree_store_->clear();

    root_node_ = std::make_shared<FileNode>();
    root_node_->name = fs::path(path).filename().string();
    root_node_->fullPath = path;
    root_node_->isDirectory = true;

    loadDirectory(root_node_);

    Gtk::TreeModel::Row rootRow = *(tree_store_->append());
    rootRow[dynamic_cast<const Gtk::TreeModelColumn<Glib::ustring>&>(
        tree_store_->get_column_record().get_column(0))] = Glib::ustring(root_node_->name);

    populateTree(rootRow, root_node_);
}

std::string FileExplorer::getSelectedFile() const {
    auto selection = tree_view_.get_selection();
    if (!selection) {
        return "";
    }

    auto iter = selection->get_selected();
    if (!iter) {
        return "";
    }

    return "";
}

void FileExplorer::onRowActivated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* /* column */) {
    auto iter = tree_store_->get_iter(path);
    if (iter) {
        signal_file_activated_.emit("");
    }
}

void FileExplorer::onSelectionChanged() {
    signal_file_selected_.emit(getSelectedFile());
}

void FileExplorer::loadDirectory(const std::shared_ptr<FileNode>& node) {
    if (!node || !fs::is_directory(node->fullPath)) {
        return;
    }

    try {
        std::vector<std::string> entries;

        for (const auto& entry : fs::directory_iterator(node->fullPath)) {
            entries.push_back(entry.path().string());
        }

        std::sort(entries.begin(), entries.end(),
            [](const std::string& a, const std::string& b) {
                bool aIsDir = fs::is_directory(a);
                bool bIsDir = fs::is_directory(b);
                if (aIsDir != bIsDir) {
                    return aIsDir > bIsDir;
                }
                return fs::path(a).filename() < fs::path(b).filename();
            });

        for (const auto& entry : entries) {
            auto child = std::make_shared<FileNode>();
            child->name = fs::path(entry).filename().string();
            child->fullPath = entry;
            child->isDirectory = fs::is_directory(entry);
            node->children.push_back(child);
        }
    } catch (const fs::filesystem_error&) {
        // Ignore directory access errors
    }
}

void FileExplorer::populateTree(const Gtk::TreeModel::Row& parentRow, const std::shared_ptr<FileNode>& node) {
    for (const auto& child : node->children) {
        auto childRow = *(tree_store_->append(parentRow.children()));
        auto col = dynamic_cast<const Gtk::TreeModelColumn<Glib::ustring>&>(
            tree_store_->get_column_record().get_column(0));
        childRow[col] = Glib::ustring(child->name);

        if (child->isDirectory && child->children.empty()) {
            loadDirectory(child);
            populateTree(childRow, child);
        }
    }
}

} // namespace xenon::ui
