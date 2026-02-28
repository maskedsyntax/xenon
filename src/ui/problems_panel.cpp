#include "ui/problems_panel.hpp"
#include <filesystem>

namespace fs = std::filesystem;

namespace xenon::ui {

ProblemsPanel::ProblemsPanel()
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0) {

    count_label_.set_text("No problems");
    count_label_.set_halign(Gtk::ALIGN_START);
    count_label_.set_margin_start(8);
    count_label_.set_margin_top(4);
    count_label_.set_margin_bottom(4);
    count_label_.get_style_context()->add_class("dim-label");
    pack_start(count_label_, false, false);

    auto* sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
    pack_start(*sep, false, false);

    store_ = Gtk::TreeStore::create(cols_);
    tree_view_.set_model(store_);
    tree_view_.set_headers_visible(false);

    auto* icon_col = Gtk::manage(new Gtk::TreeViewColumn(""));
    auto* icon_cell = Gtk::manage(new Gtk::CellRendererText());
    icon_col->pack_start(*icon_cell, false);
    icon_col->add_attribute(*icon_cell, "text", cols_.icon);
    tree_view_.append_column(*icon_col);

    auto* msg_col = Gtk::manage(new Gtk::TreeViewColumn(""));
    auto* msg_cell = Gtk::manage(new Gtk::CellRendererText());
    msg_cell->property_ellipsize() = Pango::ELLIPSIZE_END;
    msg_col->pack_start(*msg_cell, true);
    msg_col->add_attribute(*msg_cell, "text", cols_.message);
    msg_col->set_expand(true);
    tree_view_.append_column(*msg_col);

    auto* loc_col = Gtk::manage(new Gtk::TreeViewColumn(""));
    auto* loc_cell = Gtk::manage(new Gtk::CellRendererText());
    loc_cell->property_foreground() = "#888888";
    loc_col->pack_start(*loc_cell, false);
    loc_col->add_attribute(*loc_cell, "text", cols_.location);
    tree_view_.append_column(*loc_col);

    tree_view_.signal_row_activated().connect(
        sigc::mem_fun(*this, &ProblemsPanel::onRowActivated));

    scroll_.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    scroll_.add(tree_view_);
    pack_start(scroll_, true, true);

    show_all();
}

void ProblemsPanel::updateDiagnostics(const std::string& uri,
                                       const std::vector<xenon::lsp::Diagnostic>& diags) {
    if (diags.empty()) {
        all_diags_.erase(uri);
    } else {
        all_diags_[uri] = diags;
    }
    rebuild();
}

void ProblemsPanel::clearAll() {
    all_diags_.clear();
    rebuild();
}

int ProblemsPanel::totalCount() const {
    int count = 0;
    for (const auto& [uri, diags] : all_diags_) {
        count += static_cast<int>(diags.size());
    }
    return count;
}

void ProblemsPanel::rebuild() {
    store_->clear();

    int errors = 0, warnings = 0;

    for (const auto& [uri, diags] : all_diags_) {
        if (diags.empty()) continue;

        // File-level row
        auto file_row = store_->append();
        std::string path = uri;
        if (path.substr(0, 7) == "file://") path = path.substr(7);
        (*file_row)[cols_.icon]     = "";
        (*file_row)[cols_.message]  = fs::path(path).filename().string();
        (*file_row)[cols_.location] = fs::path(path).parent_path().string();
        (*file_row)[cols_.uri]      = uri;
        (*file_row)[cols_.line]     = 0;
        (*file_row)[cols_.col]      = 0;

        for (const auto& d : diags) {
            auto row = store_->append(file_row->children());
            std::string icon;
            switch (d.severity) {
                case 1: icon = "✖"; ++errors;   break;
                case 2: icon = "⚠"; ++warnings; break;
                case 3: icon = "ℹ"; break;
                default: icon = "·"; break;
            }
            (*row)[cols_.icon]     = icon;
            (*row)[cols_.message]  = d.message;
            (*row)[cols_.location] = std::to_string(d.line + 1) + ":" + std::to_string(d.col + 1);
            (*row)[cols_.uri]      = uri;
            (*row)[cols_.line]     = d.line + 1;  // 1-based for display
            (*row)[cols_.col]      = d.col + 1;
        }
    }

    tree_view_.expand_all();

    // Update count label
    if (errors == 0 && warnings == 0) {
        count_label_.set_text("No problems");
    } else {
        std::string text;
        if (errors > 0)   text += std::to_string(errors) + " error(s)";
        if (warnings > 0) {
            if (!text.empty()) text += "  ";
            text += std::to_string(warnings) + " warning(s)";
        }
        count_label_.set_text(text);
    }
}

void ProblemsPanel::onRowActivated(const Gtk::TreeModel::Path& path,
                                    Gtk::TreeViewColumn* /*col*/) {
    auto iter = store_->get_iter(path);
    if (!iter) return;

    std::string uri  = static_cast<std::string>((*iter)[cols_.uri]);
    int line = static_cast<int>((*iter)[cols_.line]);
    int col  = static_cast<int>((*iter)[cols_.col]);

    if (!uri.empty() && line > 0 && jump_cb_) {
        jump_cb_(uri, line, col);
    }
}

} // namespace xenon::ui
