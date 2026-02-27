#include "ui/command_palette.hpp"
#include <algorithm>
#include <cctype>

namespace xenon::ui {

CommandPalette::CommandPalette(Gtk::Window& parent)
    : Gtk::Dialog("Command Palette", parent, true) {

    set_default_size(520, 400);
    set_border_width(0);

    auto* content = get_content_area();
    content->set_spacing(0);

    // Search entry at top
    search_entry_.set_placeholder_text("Type a command...");
    search_entry_.set_margin_start(8);
    search_entry_.set_margin_end(8);
    search_entry_.set_margin_top(8);
    search_entry_.set_margin_bottom(8);
    search_entry_.signal_changed().connect(
        sigc::mem_fun(*this, &CommandPalette::onSearchChanged));

    // Activate on Enter
    search_entry_.signal_activate().connect([this]() {
        auto* row = list_box_.get_selected_row();
        if (!row) {
            // Select first visible row if none selected
            row = list_box_.get_row_at_index(0);
        }
        if (row) {
            onRowActivated(row);
        }
    });

    content->pack_start(search_entry_, false, false);

    auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
    content->pack_start(*sep, false, false);

    // Scrollable list of commands
    scroll_.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    scroll_.add(list_box_);
    scroll_.set_min_content_height(320);

    list_box_.set_selection_mode(Gtk::SELECTION_SINGLE);
    list_box_.signal_row_activated().connect(
        sigc::mem_fun(*this, &CommandPalette::onRowActivated));

    content->pack_start(scroll_, true, true);

    // Navigate with up/down arrow keys in search entry
    search_entry_.signal_key_press_event().connect([this](GdkEventKey* event) -> bool {
        if (event->keyval == GDK_KEY_Down) {
            auto* sel = list_box_.get_selected_row();
            int next = sel ? sel->get_index() + 1 : 0;
            auto* row = list_box_.get_row_at_index(next);
            if (row) {
                list_box_.select_row(*row);
                row->grab_focus();
                search_entry_.grab_focus();
            }
            return true;
        }
        if (event->keyval == GDK_KEY_Up) {
            auto* sel = list_box_.get_selected_row();
            if (sel && sel->get_index() > 0) {
                auto* row = list_box_.get_row_at_index(sel->get_index() - 1);
                if (row) {
                    list_box_.select_row(*row);
                    search_entry_.grab_focus();
                }
            }
            return true;
        }
        if (event->keyval == GDK_KEY_Escape) {
            response(Gtk::RESPONSE_CANCEL);
            return true;
        }
        return false;
    }, false);

    content->show_all();
}

void CommandPalette::addCommand(const std::string& name, const std::string& shortcut,
                                 std::function<void()> action) {
    commands_.push_back({name, shortcut, std::move(action)});
}

void CommandPalette::clearCommands() {
    commands_.clear();
    filtered_.clear();
}

void CommandPalette::show() {
    search_entry_.set_text("");
    filterCommands("");
    rebuildList();
    Gtk::Dialog::show();
    search_entry_.grab_focus();
}

void CommandPalette::onSearchChanged() {
    filterCommands(search_entry_.get_text().raw());
    rebuildList();
}

void CommandPalette::filterCommands(const std::string& query) {
    filtered_.clear();
    for (auto& cmd : commands_) {
        if (query.empty() || fuzzyMatch(query, cmd.name)) {
            filtered_.push_back(&cmd);
        }
    }
}

void CommandPalette::rebuildList() {
    // Remove all existing rows
    for (auto* child : list_box_.get_children()) {
        list_box_.remove(*child);
    }

    for (auto* cmd : filtered_) {
        auto* row = Gtk::manage(new Gtk::ListBoxRow());
        auto* box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
        box->set_margin_start(12);
        box->set_margin_end(12);
        box->set_margin_top(8);
        box->set_margin_bottom(8);

        auto* name_label = Gtk::manage(new Gtk::Label(cmd->name));
        name_label->set_halign(Gtk::ALIGN_START);
        name_label->set_hexpand(true);

        auto* shortcut_label = Gtk::manage(new Gtk::Label(cmd->shortcut));
        shortcut_label->set_halign(Gtk::ALIGN_END);
        shortcut_label->get_style_context()->add_class("dim-label");

        box->pack_start(*name_label, true, true);
        box->pack_end(*shortcut_label, false, false);

        row->add(*box);
        row->show_all();
        list_box_.append(*row);
    }

    // Select first row
    auto* first = list_box_.get_row_at_index(0);
    if (first) {
        list_box_.select_row(*first);
    }
}

void CommandPalette::onRowActivated(Gtk::ListBoxRow* row) {
    if (!row) return;
    int idx = row->get_index();
    if (idx >= 0 && idx < static_cast<int>(filtered_.size())) {
        auto action = filtered_[static_cast<size_t>(idx)]->action;
        response(Gtk::RESPONSE_OK);
        hide();
        if (action) {
            action();
        }
    }
}

bool CommandPalette::fuzzyMatch(const std::string& pattern, const std::string& text) {
    if (pattern.empty()) return true;

    std::string p_lower = pattern;
    std::string t_lower = text;
    std::transform(p_lower.begin(), p_lower.end(), p_lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    std::transform(t_lower.begin(), t_lower.end(), t_lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    // Simple subsequence match
    size_t pi = 0;
    for (size_t ti = 0; ti < t_lower.size() && pi < p_lower.size(); ++ti) {
        if (t_lower[ti] == p_lower[pi]) {
            ++pi;
        }
    }
    return pi == p_lower.size();
}

} // namespace xenon::ui
