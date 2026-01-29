#pragma once

#include <gtkmm.h>
#include <memory>
#include <vector>
#include "ui/editor_widget.hpp"

namespace xenon::ui {

class SplitPaneContainer : public Gtk::Box {
public:
    explicit SplitPaneContainer();
    virtual ~SplitPaneContainer() = default;

    // Get the active editor
    EditorWidget* getActiveEditor() const;

    // Get the root widget (either EditorWidget or Paned)
    Gtk::Widget* getRootWidget() const;

    // Split current editor horizontally or vertically
    void splitHorizontal();
    void splitVertical();

    // Get all editors in this pane tree
    std::vector<EditorWidget*> getAllEditors();

    // Check if can close (only one editor remains)
    bool canClose() const;

private:
    std::unique_ptr<EditorWidget> editor_;
    EditorWidget* active_editor_;
    Gtk::Widget* root_widget_;

    EditorWidget* createEditor();
    void replaceWithPaned(bool horizontal);
    void collectEditors(Gtk::Widget* widget, std::vector<EditorWidget*>& editors);
};

} // namespace xenon::ui
