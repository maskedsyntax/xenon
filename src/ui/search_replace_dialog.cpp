#include "ui/search_replace_dialog.hpp"

namespace xenon::ui {

SearchReplaceDialog::SearchReplaceDialog(Gtk::Window& parent)
    : Gtk::Dialog("Find and Replace", parent, true),
      case_sensitive_check_("Match Case"),
      regex_check_("Regular Expression"),
      prev_button_("Previous"),
      next_button_("Find Next"),
      replace_button_("Replace"),
      replace_all_button_("Replace All") {
    set_modal(false);
    set_default_size(500, 150);
    set_deletable(true);

    auto contentArea = get_content_area();
    contentArea->set_margin_top(12);
    contentArea->set_margin_bottom(12);
    contentArea->set_margin_start(12);
    contentArea->set_margin_end(12);
    contentArea->set_spacing(8);

    search_entry_.set_placeholder_text("Find...");
    replace_entry_.set_placeholder_text("Replace with...");

    search_box_.pack_start(search_entry_, true, true);
    search_box_.pack_start(prev_button_, false, false);
    search_box_.pack_start(next_button_, false, false);

    replace_box_.pack_start(replace_entry_, true, true);
    replace_box_.pack_start(replace_button_, false, false);
    replace_box_.pack_start(replace_all_button_, false, false);

    options_box_.pack_start(case_sensitive_check_, false, false);
    options_box_.pack_start(regex_check_, false, false);

    main_box_.pack_start(search_box_, false, false);
    main_box_.pack_start(replace_box_, false, false);
    main_box_.pack_start(options_box_, false, false);

    contentArea->pack_start(main_box_, false, false);
    contentArea->show_all();

    replace_box_.hide();
}

std::string SearchReplaceDialog::getSearchText() const {
    return search_entry_.get_text();
}

std::string SearchReplaceDialog::getReplaceText() const {
    return replace_entry_.get_text();
}

bool SearchReplaceDialog::isCaseSensitive() const {
    return case_sensitive_check_.get_active();
}

bool SearchReplaceDialog::isRegex() const {
    return regex_check_.get_active();
}

bool SearchReplaceDialog::isReplaceVisible() const {
    return replace_visible_;
}

void SearchReplaceDialog::showSearch() {
    replace_visible_ = false;
    replace_box_.hide();
    set_title("Find");
    search_entry_.grab_focus();
}

void SearchReplaceDialog::showSearchReplace() {
    replace_visible_ = true;
    replace_box_.show();
    set_title("Find and Replace");
    search_entry_.grab_focus();
}

} // namespace xenon::ui
