#include "ui/split_pane_container.hpp"
#include <algorithm>

namespace xenon::ui {

SplitPaneContainer::SplitPaneContainer()
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL),
      active_editor_(nullptr),
      root_widget_(nullptr) {
    set_hexpand(true);
    set_vexpand(true);

    auto editor = Gtk::manage(new EditorWidget());
    editor->signal_focus_in_event().connect([this, editor](GdkEventFocus* /* event */) {
        active_editor_ = editor;
        return false;
    });
    pack_start(*editor, true, true);
    root_widget_ = editor;
    active_editor_ = editor;
    editors_.push_back(editor);
    editor->show_all();
}

EditorWidget* SplitPaneContainer::getActiveEditor() const {
    return active_editor_;
}

Gtk::Widget* SplitPaneContainer::getRootWidget() const {
    return root_widget_;
}

std::vector<EditorWidget*> SplitPaneContainer::getAllEditors() {
    return editors_;
}

bool SplitPaneContainer::canClose() const {
    return editors_.size() <= 1;
}

void SplitPaneContainer::splitHorizontal() {
    replaceWithPaned(true);
}

void SplitPaneContainer::splitVertical() {
    replaceWithPaned(false);
}

void SplitPaneContainer::replaceWithPaned(bool horizontal) {
    if (!root_widget_) {
        return;
    }

    // Get the current widget
    auto current = root_widget_;

    // Remove from this container
    remove(*current);

    // Create paned widget
    Gtk::Paned* paned;
    if (horizontal) {
        paned = Gtk::manage(new Gtk::HPaned());
    } else {
        paned = Gtk::manage(new Gtk::VPaned());
    }

    // Add current widget to first pane
    paned->pack1(*current, true, true);

    // Create and add new editor to second pane
    auto new_editor = Gtk::manage(new EditorWidget());
    new_editor->signal_focus_in_event().connect([this, new_editor](GdkEventFocus* /* event */) {
        active_editor_ = new_editor;
        return false;
    });
    paned->pack2(*new_editor, true, true);

    // Add paned to this container
    pack_start(*paned, true, true);

    // Update state
    root_widget_ = paned;
    active_editor_ = new_editor;
    editors_.push_back(new_editor);

    show_all();

    // Schedule position setting after the window is realized
    Glib::signal_idle().connect([paned]() {
        int width = paned->get_allocated_width();
        if (width > 0) {
            paned->set_position(width / 2);
        } else {
            paned->set_position(300);
        }
        return false;
    });
}

} // namespace xenon::ui
