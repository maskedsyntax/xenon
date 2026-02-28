#pragma once

#include <gtkmm.h>
#include <functional>

namespace xenon::ui {

struct EditorSettings {
    std::string font_name    = "Monospace 11";
    std::string ui_font_name = "";  // empty = system default
    int tab_width            = 4;
    bool spaces_for_tabs     = true;
    bool show_line_numbers   = true;
    bool highlight_line      = true;
    bool word_wrap           = false;
    bool auto_indent         = true;
    bool show_right_margin   = true;
    int right_margin_col     = 100;
    std::string color_scheme = "oblivion";
};

class SettingsDialog : public Gtk::Dialog {
public:
    using ApplyCallback = std::function<void(const EditorSettings&)>;

    explicit SettingsDialog(Gtk::Window& parent);
    virtual ~SettingsDialog() = default;

    void setSettings(const EditorSettings& s);
    EditorSettings getSettings() const;

    void setApplyCallback(ApplyCallback cb) { apply_cb_ = std::move(cb); }

private:
    ApplyCallback apply_cb_;

    Gtk::Grid grid_;
    Gtk::FontButton font_btn_;       // editor font
    Gtk::FontButton ui_font_btn_;    // UI font
    Gtk::SpinButton tab_spin_;
    Gtk::CheckButton spaces_check_{"Use spaces for tabs"};
    Gtk::CheckButton line_numbers_check_{"Show line numbers"};
    Gtk::CheckButton highlight_line_check_{"Highlight current line"};
    Gtk::CheckButton word_wrap_check_{"Word wrap"};
    Gtk::CheckButton auto_indent_check_{"Auto indent"};
    Gtk::CheckButton right_margin_check_{"Show right margin"};
    Gtk::SpinButton margin_col_spin_;

    Glib::RefPtr<Gtk::Adjustment> tab_adj_;
    Glib::RefPtr<Gtk::Adjustment> margin_adj_;

    Gtk::ComboBoxText scheme_combo_;

    void addRow(int row, const std::string& label, Gtk::Widget& widget);
    void onApply();
};

} // namespace xenon::ui
