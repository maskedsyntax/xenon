#pragma once

#include <gtkmm.h>
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>

namespace xenon::ui {

struct SearchResult {
    std::string file;
    int line;       // 1-based
    int col;        // 1-based
    std::string text;   // full line content
    std::string match;  // matched portion
};

class SearchPanel : public Gtk::Box {
public:
    using FileOpenCallback = std::function<void(const std::string& path, int line, int col)>;

    explicit SearchPanel();
    virtual ~SearchPanel();

    void setWorkingDirectory(const std::string& dir) { working_dir_ = dir; }
    void setFileOpenCallback(FileOpenCallback cb) { open_cb_ = std::move(cb); }
    void focusSearch();

private:
    std::string working_dir_;
    FileOpenCallback open_cb_;

    Gtk::Box header_box_{Gtk::ORIENTATION_HORIZONTAL, 4};
    Gtk::Entry search_entry_;
    Gtk::CheckButton case_check_{"Aa"};
    Gtk::CheckButton regex_check_{".*"};
    Gtk::Label result_count_label_;
    Gtk::ScrolledWindow scroll_;
    Gtk::TreeView tree_view_;
    Glib::RefPtr<Gtk::TreeStore> tree_store_;

    struct Columns : public Gtk::TreeModel::ColumnRecord {
        Gtk::TreeModelColumn<Glib::ustring> display;
        Gtk::TreeModelColumn<std::string>   file;
        Gtk::TreeModelColumn<int>           line;
        Gtk::TreeModelColumn<int>           col;
        Columns() { add(display); add(file); add(line); add(col); }
    } cols_;

    std::thread search_thread_;
    std::atomic<bool> cancel_search_{false};
    std::mutex results_mutex_;
    std::vector<SearchResult> pending_results_;

    sigc::connection idle_conn_;

    void onSearchActivated();
    void startSearch(const std::string& query, bool caseSensitive, bool regex);
    void stopSearch();
    void onRowActivated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* col);
    void flushResults();
    bool onIdle();
};

} // namespace xenon::ui
