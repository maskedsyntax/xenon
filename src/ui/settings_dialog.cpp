#include "ui/settings_dialog.hpp"

namespace xenon::ui {

SettingsDialog::SettingsDialog(Gtk::Window& parent)
    : Gtk::Dialog("Preferences", parent, true) {

    set_default_size(480, 400);

    tab_adj_    = Gtk::Adjustment::create(4, 1, 16, 1, 4);
    margin_adj_ = Gtk::Adjustment::create(100, 40, 200, 1, 10);
    tab_spin_.set_adjustment(tab_adj_);
    margin_col_spin_.set_adjustment(margin_adj_);
    tab_spin_.set_digits(0);
    margin_col_spin_.set_digits(0);

    grid_.set_row_spacing(10);
    grid_.set_column_spacing(12);
    grid_.set_margin_start(16);
    grid_.set_margin_end(16);
    grid_.set_margin_top(16);
    grid_.set_margin_bottom(16);

    int row = 0;
    addRow(row++, "Font", font_btn_);

    auto* tab_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
    tab_box->pack_start(tab_spin_, false, false);
    tab_box->pack_start(spaces_check_, false, false);
    addRow(row++, "Tab width", *tab_box);

    addRow(row++, "", line_numbers_check_);
    addRow(row++, "", highlight_line_check_);
    addRow(row++, "", word_wrap_check_);
    addRow(row++, "", auto_indent_check_);
    addRow(row++, "", right_margin_check_);

    auto* margin_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
    auto* col_label = Gtk::manage(new Gtk::Label("Column:"));
    margin_box->pack_start(*col_label, false, false);
    margin_box->pack_start(margin_col_spin_, false, false);
    addRow(row++, "", *margin_box);

    // Color scheme
    for (const auto& s : {"oblivion", "classic", "tango", "solarized-dark", "solarized-light",
                           "kate", "cobalt", "monokai-extended"}) {
        scheme_combo_.append(s, s);
    }
    scheme_combo_.set_active_id("oblivion");
    addRow(row++, "Color scheme", scheme_combo_);

    auto* content = get_content_area();
    content->pack_start(grid_, true, true);
    content->show_all();

    add_button("Cancel", Gtk::RESPONSE_CANCEL);
    add_button("Apply",  Gtk::RESPONSE_APPLY);
    add_button("OK",     Gtk::RESPONSE_OK);

    signal_response().connect([this](int resp) {
        if (resp == Gtk::RESPONSE_APPLY || resp == Gtk::RESPONSE_OK) {
            onApply();
        }
        if (resp == Gtk::RESPONSE_OK) {
            hide();
        }
        if (resp == Gtk::RESPONSE_CANCEL) {
            hide();
        }
    });
}

void SettingsDialog::addRow(int row, const std::string& label, Gtk::Widget& widget) {
    if (!label.empty()) {
        auto* lbl = Gtk::manage(new Gtk::Label(label + ":"));
        lbl->set_halign(Gtk::ALIGN_END);
        grid_.attach(*lbl, 0, row, 1, 1);
        grid_.attach(widget, 1, row, 1, 1);
    } else {
        grid_.attach(widget, 1, row, 1, 1);
    }
}

void SettingsDialog::setSettings(const EditorSettings& s) {
    font_btn_.set_font_name(s.font_name);
    tab_adj_->set_value(s.tab_width);
    spaces_check_.set_active(s.spaces_for_tabs);
    line_numbers_check_.set_active(s.show_line_numbers);
    highlight_line_check_.set_active(s.highlight_line);
    word_wrap_check_.set_active(s.word_wrap);
    auto_indent_check_.set_active(s.auto_indent);
    right_margin_check_.set_active(s.show_right_margin);
    margin_adj_->set_value(s.right_margin_col);
    scheme_combo_.set_active_id(s.color_scheme);
}

EditorSettings SettingsDialog::getSettings() const {
    EditorSettings s;
    s.font_name       = font_btn_.get_font_name();
    s.tab_width       = static_cast<int>(tab_adj_->get_value());
    s.spaces_for_tabs = spaces_check_.get_active();
    s.show_line_numbers = line_numbers_check_.get_active();
    s.highlight_line  = highlight_line_check_.get_active();
    s.word_wrap       = word_wrap_check_.get_active();
    s.auto_indent     = auto_indent_check_.get_active();
    s.show_right_margin = right_margin_check_.get_active();
    s.right_margin_col  = static_cast<int>(margin_adj_->get_value());
    s.color_scheme    = scheme_combo_.get_active_id().raw();
    return s;
}

void SettingsDialog::onApply() {
    if (apply_cb_) {
        apply_cb_(getSettings());
    }
}

} // namespace xenon::ui
