#pragma once

#include <gtkmm.h>
#include <string>
#include <functional>
#include <vector>

namespace xenon::ui {

class BreadcrumbBar : public Gtk::Box {
public:
    using DirCallback = std::function<void(const std::string& path)>;

    BreadcrumbBar();
    virtual ~BreadcrumbBar() = default;

    void setPath(const std::string& filepath);
    void setDirCallback(DirCallback cb) { dir_cb_ = std::move(cb); }

private:
    DirCallback dir_cb_;
    std::string current_path_;

    void rebuild();
};

} // namespace xenon::ui
