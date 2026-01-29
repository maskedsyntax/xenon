#include "ui/split_pane_container.hpp"
#include <algorithm>

namespace xenon::ui {

SplitPaneContainer::SplitPaneContainer()
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL),
      editor_(std::make_unique<EditorWidget>()),
      active_editor_(editor_.get()),
      root_widget_(editor_.get()) {
    set_hexpand(true);
    set_vexpand(true);
    pack_start(*editor_, true, true);
    editor_->show_all();
}

EditorWidget* SplitPaneContainer::getActiveEditor() const {
    return active_editor_;
}

Gtk::Widget* SplitPaneContainer::getRootWidget() const {
    return root_widget_;
}

std::vector<EditorWidget*> SplitPaneContainer::getAllEditors() {
    std::vector<EditorWidget*> result;
    if (editor_) {
        result.push_back(editor_.get());
    }
    // Collect from any paned children
    for (auto child : get_children()) {
        collectEditors(child, result);
    }
    return result;
}

bool SplitPaneContainer::canClose() const {
    auto all = const_cast<SplitPaneContainer*>(this)->getAllEditors();
    return all.size() <= 1;
}

EditorWidget* SplitPaneContainer::createEditor() {
    auto new_editor = std::make_unique<EditorWidget>();
    auto ptr = new_editor.get();
    // Note: We don't store this because it will be managed by the paned widget
    // The paned widget becomes the new child of this box
    editor_ = std::move(new_editor);
    return ptr;
}

void SplitPaneContainer::splitHorizontal() {
    replaceWithPaned(true);
}

void SplitPaneContainer::splitVertical() {
    replaceWithPaned(false);
}

void SplitPaneContainer::replaceWithPaned(bool horizontal) {
    if (!editor_) {
        return;
    }

    // Create paned widget
    Gtk::Paned* paned;
    if (horizontal) {
        paned = Gtk::manage(new Gtk::HPaned());
    } else {
        paned = Gtk::manage(new Gtk::VPaned());
    }

    // First pane: current editor
    paned->pack1(*editor_, true, true);

    // Second pane: new editor
    auto new_editor = Gtk::manage(new EditorWidget());
    active_editor_ = new_editor;
    paned->pack2(*new_editor, true, true);

    // Remove the old editor from this container
    remove(*editor_);

    // Add the paned widget
    pack_start(*paned, true, true);
    root_widget_ = paned;

    // Release ownership of old editor from unique_ptr since paned now owns it
    editor_.release();

    paned->set_position(paned->get_allocated_width() / 2);
    show_all();
}

void SplitPaneContainer::collectEditors(Gtk::Widget* widget, std::vector<EditorWidget*>& editors) {
    auto paned = dynamic_cast<Gtk::Paned*>(widget);
    if (paned) {
        auto child1 = paned->get_child1();
        auto child2 = paned->get_child2();

        if (child1) {
            auto editor = dynamic_cast<EditorWidget*>(child1);
            if (editor) {
                editors.push_back(editor);
            } else {
                collectEditors(child1, editors);
            }
        }

        if (child2) {
            auto editor = dynamic_cast<EditorWidget*>(child2);
            if (editor) {
                editors.push_back(editor);
            } else {
                collectEditors(child2, editors);
            }
        }
    } else {
        auto editor = dynamic_cast<EditorWidget*>(widget);
        if (editor) {
            editors.push_back(editor);
        }
    }
}

} // namespace xenon::ui
