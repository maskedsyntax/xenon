#pragma once

#include <gtkmm.h>
#include <functional>
#include <string>
#include <vector>

namespace xenon::ui {

struct Command {
    std::string name;
    std::string shortcut;
    std::function<void()> action;
};

class CommandPalette : public Gtk::Dialog {
public:
    explicit CommandPalette(Gtk::Window& parent);
    virtual ~CommandPalette() = default;

    void addCommand(const std::string& name, const std::string& shortcut,
                    std::function<void()> action);
    void clearCommands();
    void show();

private:
    std::vector<Command> commands_;
    std::vector<Command*> filtered_;

    Gtk::Entry search_entry_;
    Gtk::ScrolledWindow scroll_;
    Gtk::ListBox list_box_;

    void onSearchChanged();
    void filterCommands(const std::string& query);
    void rebuildList();
    void onRowActivated(Gtk::ListBoxRow* row);

    static bool fuzzyMatch(const std::string& pattern, const std::string& text);
};

} // namespace xenon::ui
