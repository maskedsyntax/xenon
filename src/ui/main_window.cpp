#include "ui/main_window.hpp"
#include "core/file_manager.hpp"

namespace xenon::ui {

MainWindow::MainWindow(Glib::RefPtr<Gtk::Application> app)
    : Gtk::ApplicationWindow(app), app_(app) {
    set_title("Xenon");
    set_default_size(1200, 800);

    setupUI();
    setupMenuBar();
    createNewTab();

    show_all_children();
}

void MainWindow::setupUI() {
    main_box_.set_margin_top(0);
    main_box_.set_margin_bottom(0);
    main_box_.set_margin_start(0);
    main_box_.set_margin_end(0);

    main_box_.pack_start(menubar_, false, false);
    main_box_.pack_start(notebook_, true, true);
    main_box_.pack_end(statusbar_, false, false);

    add(main_box_);
}

void MainWindow::setupMenuBar() {
    auto fileMenu = Gtk::manage(new Gtk::Menu());

    auto newItem = Gtk::manage(new Gtk::MenuItem("_New", true));
    newItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onFileNew));
    fileMenu->append(*newItem);

    auto openItem = Gtk::manage(new Gtk::MenuItem("_Open", true));
    openItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onFileOpen));
    fileMenu->append(*openItem);

    auto saveItem = Gtk::manage(new Gtk::MenuItem("_Save", true));
    saveItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onFileSave));
    fileMenu->append(*saveItem);

    fileMenu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

    auto quitItem = Gtk::manage(new Gtk::MenuItem("_Quit", true));
    quitItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onFileQuit));
    fileMenu->append(*quitItem);

    auto fileMenuitem = Gtk::manage(new Gtk::MenuItem("_File", true));
    fileMenuitem->set_submenu(*fileMenu);

    menubar_.append(*fileMenuitem);
    menubar_.show_all();
}

void MainWindow::createNewTab() {
    auto editor = std::make_unique<EditorWidget>();
    int pageNum = notebook_.append_page(*editor, "Untitled");
    notebook_.set_current_page(pageNum);
    editors_.push_back(std::move(editor));
}

void MainWindow::onFileNew() {
    createNewTab();
}

void MainWindow::onFileOpen() {
    Gtk::FileChooserDialog dialog("Open File", Gtk::FILE_CHOOSER_ACTION_OPEN);
    dialog.set_transient_for(*this);

    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

    if (dialog.run() == Gtk::RESPONSE_OK) {
        std::string filename = dialog.get_filename();
        try {
            std::string content = xenon::core::FileManager::readFile(filename);
            createNewTab();

            EditorWidget* editor = dynamic_cast<EditorWidget*>(
                notebook_.get_nth_page(notebook_.get_current_page())
            );

            if (editor) {
                editor->setContent(content);
                notebook_.set_tab_label_text(*editor,
                    xenon::core::FileManager::getFileName(filename));
            }
        } catch (const std::exception& e) {
            Gtk::MessageDialog errorDialog(*this, e.what(),
                false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
            errorDialog.run();
        }
    }
}

void MainWindow::onFileSave() {
    EditorWidget* editor = dynamic_cast<EditorWidget*>(
        notebook_.get_nth_page(notebook_.get_current_page())
    );

    if (editor) {
        editor->saveFile();
    }
}

void MainWindow::onFileQuit() {
    close();
}

bool MainWindow::on_delete_event(GdkEventAny* /* any_event */) {
    app_->quit();
    return true;
}

bool MainWindow::on_key_press_event(GdkEventKey* event) {
    if (event->state & GDK_CONTROL_MASK) {
        if (event->keyval == GDK_KEY_s) {
            onFileSave();
            return true;
        } else if (event->keyval == GDK_KEY_n) {
            onFileNew();
            return true;
        } else if (event->keyval == GDK_KEY_o) {
            onFileOpen();
            return true;
        }
    }

    return Gtk::ApplicationWindow::on_key_press_event(event);
}

} // namespace xenon::ui
