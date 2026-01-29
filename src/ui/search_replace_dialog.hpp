#pragma once

#include <gtkmm.h>
#include <string>
#include <memory>

namespace xenon::ui {

class SearchReplaceDialog : public Gtk::Dialog {
public:
    explicit SearchReplaceDialog(Gtk::Window& parent);
    virtual ~SearchReplaceDialog() = default;

    std::string getSearchText() const;
    std::string getReplaceText() const;
    bool isCaseSensitive() const;
    bool isRegex() const;
    bool isReplaceVisible() const;

    void showSearch();
    void showSearchReplace();

    sigc::signal<void> signal_find_next();
    sigc::signal<void> signal_find_previous();
    sigc::signal<void> signal_replace();
    sigc::signal<void> signal_replace_all();

private:
    Gtk::Box main_box_{Gtk::ORIENTATION_VERTICAL, 6};
    Gtk::Box search_box_{Gtk::ORIENTATION_HORIZONTAL, 6};
    Gtk::Box replace_box_{Gtk::ORIENTATION_HORIZONTAL, 6};
    Gtk::Box options_box_{Gtk::ORIENTATION_HORIZONTAL, 6};

    Gtk::Entry search_entry_;
    Gtk::Entry replace_entry_;
    Gtk::CheckButton case_sensitive_check_;
    Gtk::CheckButton regex_check_;
    Gtk::Button prev_button_;
    Gtk::Button next_button_;
    Gtk::Button replace_button_;
    Gtk::Button replace_all_button_;

    bool replace_visible_ = false;

    sigc::signal<void> signal_find_next_;
    sigc::signal<void> signal_find_previous_;
    sigc::signal<void> signal_replace_;
    sigc::signal<void> signal_replace_all_;

    void toggleReplace();
};

} // namespace xenon::ui
