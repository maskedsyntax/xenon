#pragma once

#include <gtkmm.h>
#include <vector>
#include "ui/editor_widget.hpp"

namespace xenon::ui {

class SplitPaneContainer : public Gtk::Box {
public:
    explicit SplitPaneContainer();
    virtual ~SplitPaneContainer() = default;

    EditorWidget* getActiveEditor() const;
    Gtk::Widget* getRootWidget() const;

    void splitHorizontal();
    void splitVertical();

    std::vector<EditorWidget*> getAllEditors();
    bool canClose() const;

private:
    std::vector<EditorWidget*> editors_;
    EditorWidget* active_editor_;
    Gtk::Widget* root_widget_;

    void replaceWithPaned(bool horizontal);
};

} // namespace xenon::ui
