#pragma once

#include <gtkmm.h>
#include <string>
#include <vector>
#include <functional>
#include "lsp/lsp_client.hpp"

namespace xenon::ui {

class ProblemsPanel : public Gtk::Box {
public:
    using JumpCallback = std::function<void(const std::string& uri, int line, int col)>;

    ProblemsPanel();
    virtual ~ProblemsPanel() = default;

    void updateDiagnostics(const std::string& uri,
                           const std::vector<xenon::lsp::Diagnostic>& diags);
    void setJumpCallback(JumpCallback cb) { jump_cb_ = std::move(cb); }
    void clearAll();

    int totalCount() const;

private:
    JumpCallback jump_cb_;

    // uri -> diagnostics
    std::unordered_map<std::string, std::vector<xenon::lsp::Diagnostic>> all_diags_;

    Gtk::Label count_label_;
    Gtk::ScrolledWindow scroll_;
    Gtk::TreeView tree_view_;
    Glib::RefPtr<Gtk::TreeStore> store_;

    struct Cols : public Gtk::TreeModel::ColumnRecord {
        Gtk::TreeModelColumn<Glib::ustring> icon;
        Gtk::TreeModelColumn<Glib::ustring> message;
        Gtk::TreeModelColumn<Glib::ustring> location;
        Gtk::TreeModelColumn<std::string>   uri;
        Gtk::TreeModelColumn<int>           line;
        Gtk::TreeModelColumn<int>           col;
        Cols() { add(icon); add(message); add(location); add(uri); add(line); add(col); }
    } cols_;

    void rebuild();
    void onRowActivated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* col);
};

} // namespace xenon::ui
