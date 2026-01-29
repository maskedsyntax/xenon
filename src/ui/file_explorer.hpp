#pragma once

#include <gtkmm.h>
#include <string>
#include <memory>
#include <vector>
#include <filesystem>

namespace xenon::ui {

class FileExplorer : public Gtk::ScrolledWindow {
public:
    FileExplorer();
    virtual ~FileExplorer() = default;

    void setRootDirectory(const std::string& path);
    std::string getSelectedFile() const;

    sigc::signal<void, std::string>& signal_file_selected() { return signal_file_selected_; }
    sigc::signal<void, std::string>& signal_file_activated() { return signal_file_activated_; }

private:
    struct FileNode {
        std::string name;
        std::string fullPath;
        bool isDirectory;
        std::vector<std::shared_ptr<FileNode>> children;
    };

    Gtk::TreeView tree_view_;
    Glib::RefPtr<Gtk::TreeStore> tree_store_;
    std::shared_ptr<FileNode> root_node_;

    sigc::signal<void, std::string> signal_file_selected_;
    sigc::signal<void, std::string> signal_file_activated_;

    void onRowActivated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);
    void onSelectionChanged();
    void loadDirectory(const std::shared_ptr<FileNode>& node);
    void populateTree(const Gtk::TreeModel::Row& parentRow, const std::shared_ptr<FileNode>& node);
};

} // namespace xenon::ui
