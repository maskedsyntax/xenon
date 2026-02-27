#pragma once

#include <gtkmm.h>
#include <string>

namespace xenon::ui {

class StatusBar : public Gtk::Box {
public:
    StatusBar();
    virtual ~StatusBar() = default;

    void setCursorPosition(int line, int col);
    void setLanguage(const std::string& lang);
    void setEncoding(const std::string& encoding);
    void setLineEnding(const std::string& lineEnding);
    void setMessage(const std::string& message);
    void clearMessage();

private:
    Gtk::Label message_label_;
    Gtk::Separator sep1_;
    Gtk::Label position_label_;
    Gtk::Separator sep2_;
    Gtk::Label language_label_;
    Gtk::Separator sep3_;
    Gtk::Label encoding_label_;
    Gtk::Separator sep4_;
    Gtk::Label line_ending_label_;
};

} // namespace xenon::ui
