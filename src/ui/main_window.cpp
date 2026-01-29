#include "ui/main_window.hpp"
#include "core/file_manager.hpp"
#include <filesystem>

namespace fs = std::filesystem;

namespace xenon::ui {

MainWindow::MainWindow(Glib::RefPtr<Gtk::Application> app)
    : Gtk::ApplicationWindow(app), app_(app) {
    set_title("Xenon");
    set_default_size(1200, 800);

    working_directory_ = fs::current_path().string();

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

    // Setup search dialog
    search_dialog_ = std::make_unique<SearchReplaceDialog>(*this);

    // Setup quick open dialog
    quick_open_dialog_ = std::make_unique<QuickOpenDialog>(*this);
    quick_open_dialog_->setWorkingDirectory(working_directory_);

    content_box_.pack_start(notebook_, true, true);
    main_box_.pack_start(content_box_, true, true);
    main_box_.pack_end(statusbar_, false, false);

    add(main_box_);
}

void MainWindow::setupMenuBar() {
    // File menu
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

    auto saveAsItem = Gtk::manage(new Gtk::MenuItem("Save _As", true));
    saveAsItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onFileSaveAs));
    fileMenu->append(*saveAsItem);

    fileMenu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

    auto quitItem = Gtk::manage(new Gtk::MenuItem("_Quit", true));
    quitItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onFileQuit));
    fileMenu->append(*quitItem);

    auto fileMenuitem = Gtk::manage(new Gtk::MenuItem("_File", true));
    fileMenuitem->set_submenu(*fileMenu);

    // Edit menu
    auto editMenu = Gtk::manage(new Gtk::Menu());

    auto findItem = Gtk::manage(new Gtk::MenuItem("_Find", true));
    findItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onEditFind));
    editMenu->append(*findItem);

    auto findReplaceItem = Gtk::manage(new Gtk::MenuItem("Find and _Replace", true));
    findReplaceItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onEditFindReplace));
    editMenu->append(*findReplaceItem);

    auto editMenuitem = Gtk::manage(new Gtk::MenuItem("_Edit", true));
    editMenuitem->set_submenu(*editMenu);

    menubar_.append(*fileMenuitem);
    menubar_.append(*editMenuitem);
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
        } else if (event->keyval == GDK_KEY_p) {
            onQuickOpen();
            return true;
        } else if (event->keyval == GDK_KEY_f) {
            onEditFind();
            return true;
        } else if (event->keyval == GDK_KEY_h) {
            onEditFindReplace();
            return true;
        }
    }

    return Gtk::ApplicationWindow::on_key_press_event(event);
}

void MainWindow::onFileSaveAs() {
    EditorWidget* editor = dynamic_cast<EditorWidget*>(
        notebook_.get_nth_page(notebook_.get_current_page())
    );

    if (!editor) {
        return;
    }

    Gtk::FileChooserDialog dialog("Save File As", Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.set_transient_for(*this);

    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);

    if (dialog.run() == Gtk::RESPONSE_OK) {
        std::string filename = dialog.get_filename();
        editor->setFilePath(filename);
        editor->saveFile();
        notebook_.set_tab_label_text(*editor, xenon::core::FileManager::getFileName(filename));
    }
}

void MainWindow::onEditFind() {
    search_dialog_->showSearch();
    search_dialog_->show();
}

void MainWindow::onEditFindReplace() {
    search_dialog_->showSearchReplace();
    search_dialog_->show();
}


void MainWindow::onQuickOpen() {
    quick_open_dialog_->show();
    if (quick_open_dialog_->run() == Gtk::RESPONSE_OK) {
        std::string filepath = quick_open_dialog_->getSelectedFile();
        if (!filepath.empty()) {
            try {
                std::string content = xenon::core::FileManager::readFile(filepath);
                createNewTab();

                EditorWidget* editor = dynamic_cast<EditorWidget*>(
                    notebook_.get_nth_page(notebook_.get_current_page())
                );

                if (editor) {
                    editor->setContent(content);
                    editor->setFilePath(filepath);
                    notebook_.set_tab_label_text(*editor,
                        xenon::core::FileManager::getFileName(filepath));
                }
            } catch (const std::exception& e) {
                Gtk::MessageDialog errorDialog(*this, e.what(),
                    false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
                errorDialog.run();
            }
        }
    }
    quick_open_dialog_->hide();
}


} // namespace xenon::ui
