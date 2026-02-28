#include "ui/breadcrumb_bar.hpp"
#include <filesystem>

namespace fs = std::filesystem;

namespace xenon::ui {

BreadcrumbBar::BreadcrumbBar()
    : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0) {
    get_style_context()->add_class("xenon-breadcrumbs");
    set_margin_start(4);
}

void BreadcrumbBar::setPath(const std::string& filepath) {
    current_path_ = filepath;
    rebuild();
}

void BreadcrumbBar::rebuild() {
    // Remove all children
    for (auto* child : get_children()) {
        remove(*child);
    }

    if (current_path_.empty()) return;

    fs::path p(current_path_);
    std::vector<fs::path> segments;

    // Collect path segments from root to file
    fs::path tmp = p;
    while (!tmp.empty() && tmp != tmp.parent_path()) {
        segments.push_back(tmp);
        tmp = tmp.parent_path();
    }
    if (!tmp.empty()) segments.push_back(tmp);
    std::reverse(segments.begin(), segments.end());

    // Limit to last 4 segments to avoid overflow
    size_t start = segments.size() > 4 ? segments.size() - 4 : 0;

    if (start > 0) {
        auto* ellipsis = Gtk::manage(new Gtk::Label("…"));
        ellipsis->get_style_context()->add_class("breadcrumb-sep");
        pack_start(*ellipsis, false, false);
        auto* sep = Gtk::manage(new Gtk::Label(" › "));
        sep->get_style_context()->add_class("breadcrumb-sep");
        pack_start(*sep, false, false);
    }

    for (size_t i = start; i < segments.size(); ++i) {
        const auto& seg = segments[i];
        std::string name = seg.filename().empty() ? seg.string() : seg.filename().string();
        bool is_last = (i == segments.size() - 1);

        auto* btn = Gtk::manage(new Gtk::Button(name));
        btn->set_relief(Gtk::RELIEF_NONE);
        btn->get_style_context()->add_class("breadcrumb-btn");
        if (is_last) {
            btn->get_style_context()->add_class("breadcrumb-active");
        }

        std::string seg_path = seg.string();
        btn->signal_clicked().connect([this, seg_path]() {
            if (dir_cb_) dir_cb_(seg_path);
        });

        pack_start(*btn, false, false);

        if (!is_last) {
            auto* sep = Gtk::manage(new Gtk::Label(" › "));
            sep->get_style_context()->add_class("breadcrumb-sep");
            pack_start(*sep, false, false);
        }
    }

    show_all();
}

} // namespace xenon::ui
