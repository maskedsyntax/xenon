#pragma once

#include <gtkmm.h>
#include <string>
#include <vector>
#include <filesystem>

namespace xenon::ui {

struct FileEntry {
    std::string path;
    std::string displayName;
    bool isDirectory;
};

class QuickOpenDialog : public Gtk::Dialog {
public:
    explicit QuickOpenDialog(Gtk::Window& parent);
    virtual ~QuickOpenDialog() = default;

    void setWorkingDirectory(const std::string& path);
    std::string getSelectedFile() const;

private:
    Gtk::SearchEntry search_entry_;
    Gtk::ListBox file_list_;
    Gtk::ScrolledWindow scroll_;
    std::vector<FileEntry> all_files_;
    std::string working_dir_;
    std::string selected_file_;
    std::vector<FileEntry> current_results_; // Cache filtered results

    void loadFiles();
    void onSearchChanged();
    void onRowActivated(Gtk::ListBoxRow* row);
    std::vector<FileEntry> fuzzyFilter(const std::string& query);
    int calculateScore(const std::string& filename, const std::string& query);
    void updateFileList(const std::vector<FileEntry>& files);
};

} // namespace xenon::ui
