#pragma once

#include <gtkmm.h>
#include <string>

namespace xenon::ui {

class ThemeManager {
public:
    static ThemeManager& instance();

    void applyDarkTheme(Glib::RefPtr<Gdk::Screen> screen);

    // Change the application-wide UI font (empty string = system default)
    static void applyUIFont(const std::string& font_name);

private:
    ThemeManager() = default;
    Glib::RefPtr<Gtk::CssProvider> css_provider_;

    static const char* DARK_THEME_CSS;
};

} // namespace xenon::ui
