#include "ui/status_bar.hpp"

namespace xenon::ui {

StatusBar::StatusBar()
    : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0),
      sep0_(Gtk::ORIENTATION_VERTICAL),
      sep1_(Gtk::ORIENTATION_VERTICAL),
      sep2_(Gtk::ORIENTATION_VERTICAL),
      sep3_(Gtk::ORIENTATION_VERTICAL),
      sep4_(Gtk::ORIENTATION_VERTICAL) {

    get_style_context()->add_class("xenon-statusbar");
    set_margin_top(0);
    set_margin_bottom(0);

    // Left side: message
    message_label_.set_text("");
    message_label_.set_halign(Gtk::ALIGN_START);
    message_label_.set_margin_start(8);
    message_label_.set_margin_end(8);
    message_label_.get_style_context()->add_class("statusbar-message");
    pack_start(message_label_, false, false);

    // Git branch (left side, after message)
    sep0_.set_margin_top(4);
    sep0_.set_margin_bottom(4);
    pack_start(sep0_, false, false);
    git_label_.set_margin_start(12);
    git_label_.set_margin_end(12);
    git_label_.get_style_context()->add_class("statusbar-item");
    pack_start(git_label_, false, false);

    // Right-side info labels
    for (auto* sep : {&sep1_, &sep2_, &sep3_, &sep4_}) {
        sep->set_margin_top(4);
        sep->set_margin_bottom(4);
    }

    pack_end(line_ending_label_, false, false);
    pack_end(sep4_, false, false);
    pack_end(encoding_label_, false, false);
    pack_end(sep3_, false, false);
    pack_end(language_label_, false, false);
    pack_end(sep2_, false, false);
    pack_end(position_label_, false, false);
    pack_end(sep1_, false, false);

    for (auto* lbl : {&position_label_, &language_label_, &encoding_label_, &line_ending_label_}) {
        lbl->set_margin_start(12);
        lbl->set_margin_end(12);
        lbl->get_style_context()->add_class("statusbar-item");
    }

    setCursorPosition(1, 1);
    setLanguage("Plain Text");
    setEncoding("UTF-8");
    setLineEnding("LF");

    show_all();
    git_label_.hide();
    sep0_.hide();
}

void StatusBar::setCursorPosition(int line, int col) {
    position_label_.set_text("Ln " + std::to_string(line) + ", Col " + std::to_string(col));
}

void StatusBar::setLanguage(const std::string& lang) {
    language_label_.set_text(lang);
}

void StatusBar::setEncoding(const std::string& encoding) {
    encoding_label_.set_text(encoding);
}

void StatusBar::setLineEnding(const std::string& lineEnding) {
    line_ending_label_.set_text(lineEnding);
}

void StatusBar::setMessage(const std::string& message) {
    message_label_.set_text(message);
}

void StatusBar::clearMessage() {
    message_label_.set_text("");
}

void StatusBar::setGitBranch(const std::string& branch) {
    if (branch.empty()) {
        git_label_.hide();
        sep0_.hide();
    } else {
        git_label_.set_text("\uf126 " + branch);  // branch icon (nerd font) or plain
        git_label_.show();
        sep0_.show();
    }
}

} // namespace xenon::ui
