#pragma once

#include <gtkmm.h>
#include <string>
#include <filesystem>

namespace xenon::ui {

class FileExplorer : public Gtk::ScrolledWindow {
public:
    FileExplorer();
    virtual ~FileExplorer() = default;

    void setRootDirectory(const std::string& path);
    std::string getSelectedFile();
    void refresh();

    sigc::signal<void, std::string> signal_file_activated() { return signal_file_activated_; }

protected:
    // Tree model columns: Name, FullPath, IsDirectory
    class ModelColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        ModelColumns() { add(col_name); add(col_path); add(col_is_dir); }

        Gtk::TreeModelColumn<Glib::ustring> col_name;
        Gtk::TreeModelColumn<Glib::ustring> col_path;
        Gtk::TreeModelColumn<bool> col_is_dir;
    };

    ModelColumns columns_;
    Gtk::TreeView tree_view_;
    Glib::RefPtr<Gtk::TreeStore> tree_store_;
    std::string root_path_;

    sigc::signal<void, std::string> signal_file_activated_;

    void onRowActivated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);
    void populateDirectory(const Gtk::TreeModel::Row* parent, const std::string& path);
};

} // namespace xenon::ui
