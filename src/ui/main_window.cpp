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

    // Create accel group for keyboard shortcuts
    accel_group_ = Gtk::AccelGroup::create();
    add_accel_group(accel_group_);

    main_box_.pack_start(menubar_, false, false);

    // Setup search dialog
    search_dialog_ = std::make_unique<SearchReplaceDialog>(*this);
    search_dialog_->signal_find_next().connect(sigc::mem_fun(*this, &MainWindow::onFindNext));
    search_dialog_->signal_find_previous().connect(sigc::mem_fun(*this, &MainWindow::onFindPrevious));
    search_dialog_->signal_replace().connect(sigc::mem_fun(*this, &MainWindow::onReplace));
    search_dialog_->signal_replace_all().connect(sigc::mem_fun(*this, &MainWindow::onReplaceAll));

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
    fileMenu->set_accel_group(accel_group_);

    auto newItem = Gtk::manage(new Gtk::MenuItem("_New"));
    newItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onFileNew));
    newItem->add_accelerator("activate", accel_group_, GDK_KEY_n, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    fileMenu->append(*newItem);

    auto openItem = Gtk::manage(new Gtk::MenuItem("_Open File"));
    openItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onFileOpen));
    openItem->add_accelerator("activate", accel_group_, GDK_KEY_o, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    fileMenu->append(*openItem);

    auto openFolderItem = Gtk::manage(new Gtk::MenuItem("Open _Folder"));
    openFolderItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onOpenFolder));
    fileMenu->append(*openFolderItem);

    auto saveItem = Gtk::manage(new Gtk::MenuItem("_Save"));
    saveItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onFileSave));
    saveItem->add_accelerator("activate", accel_group_, GDK_KEY_s, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    fileMenu->append(*saveItem);

    auto saveAsItem = Gtk::manage(new Gtk::MenuItem("Save _As"));
    saveAsItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onFileSaveAs));
    fileMenu->append(*saveAsItem);

    fileMenu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

    auto quitItem = Gtk::manage(new Gtk::MenuItem("_Quit"));
    quitItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onFileQuit));
    fileMenu->append(*quitItem);

    auto fileMenuitem = Gtk::manage(new Gtk::MenuItem("_File"));
    fileMenuitem->set_submenu(*fileMenu);

    // Edit menu
    auto editMenu = Gtk::manage(new Gtk::Menu());
    editMenu->set_accel_group(accel_group_);

    auto quickOpenItem = Gtk::manage(new Gtk::MenuItem("Quick _Open"));
    quickOpenItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onQuickOpen));
    quickOpenItem->add_accelerator("activate", accel_group_, GDK_KEY_p, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    editMenu->append(*quickOpenItem);

    editMenu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

    auto findItem = Gtk::manage(new Gtk::MenuItem("_Find"));
    findItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onEditFind));
    findItem->add_accelerator("activate", accel_group_, GDK_KEY_f, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    editMenu->append(*findItem);

    auto findReplaceItem = Gtk::manage(new Gtk::MenuItem("Find and _Replace"));
    findReplaceItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onEditFindReplace));
    findReplaceItem->add_accelerator("activate", accel_group_, GDK_KEY_h, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    editMenu->append(*findReplaceItem);

    auto editMenuitem = Gtk::manage(new Gtk::MenuItem("_Edit"));
    editMenuitem->set_submenu(*editMenu);

    // View menu
    auto viewMenu = Gtk::manage(new Gtk::Menu());
    viewMenu->set_accel_group(accel_group_);

    auto splitHorizontalItem = Gtk::manage(new Gtk::MenuItem("Split _Horizontally"));
    splitHorizontalItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onSplitHorizontal));
    splitHorizontalItem->add_accelerator("activate", accel_group_, GDK_KEY_h, Gdk::MOD1_MASK, Gtk::ACCEL_VISIBLE);
    viewMenu->append(*splitHorizontalItem);

    auto splitVerticalItem = Gtk::manage(new Gtk::MenuItem("Split _Vertically"));
    splitVerticalItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onSplitVertical));
    splitVerticalItem->add_accelerator("activate", accel_group_, GDK_KEY_v, Gdk::MOD1_MASK, Gtk::ACCEL_VISIBLE);
    viewMenu->append(*splitVerticalItem);

    viewMenu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

    auto selectLangItem = Gtk::manage(new Gtk::MenuItem("Set _Language"));
    selectLangItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onSelectLanguage));
    viewMenu->append(*selectLangItem);

    auto viewMenuitem = Gtk::manage(new Gtk::MenuItem("_View"));
    viewMenuitem->set_submenu(*viewMenu);

    menubar_.append(*fileMenuitem);
    menubar_.append(*editMenuitem);
    menubar_.append(*viewMenuitem);
    menubar_.show_all();
}

void MainWindow::createNewTab() {
    auto split_pane = Gtk::manage(new SplitPaneContainer());
    int pageNum = notebook_.append_page(*split_pane, "Untitled");
    notebook_.set_current_page(pageNum);
    split_panes_.push_back(split_pane);
}

SplitPaneContainer* MainWindow::getCurrentSplitPane() {
    int pageNum = notebook_.get_current_page();
    if (pageNum >= 0 && pageNum < static_cast<int>(split_panes_.size())) {
        return split_panes_[pageNum];
    }
    return nullptr;
}

EditorWidget* MainWindow::getActiveEditor() {
    auto split_pane = getCurrentSplitPane();
    if (split_pane) {
        return split_pane->getActiveEditor();
    }
    return nullptr;
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

            EditorWidget* editor = getActiveEditor();
            if (editor) {
                editor->setContent(content);
                editor->setFilePath(filename);
                int pageNum = notebook_.get_current_page();
                notebook_.set_tab_label_text(
                    *notebook_.get_nth_page(pageNum),
                    xenon::core::FileManager::getFileName(filename));
            }
        } catch (const std::exception& e) {
            Gtk::MessageDialog errorDialog(*this, e.what(),
                false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
            errorDialog.run();
        }
    }
}

void MainWindow::onOpenFolder() {
    Gtk::FileChooserDialog dialog("Open Folder", Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
    dialog.set_transient_for(*this);

    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

    if (dialog.run() == Gtk::RESPONSE_OK) {
        std::string folder = dialog.get_filename();
        working_directory_ = folder;
        quick_open_dialog_->setWorkingDirectory(folder);
    }
}

void MainWindow::onFileSave() {
    EditorWidget* editor = getActiveEditor();
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

void MainWindow::onFileSaveAs() {
    EditorWidget* editor = getActiveEditor();
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
        int pageNum = notebook_.get_current_page();
        notebook_.set_tab_label_text(
            *notebook_.get_nth_page(pageNum),
            xenon::core::FileManager::getFileName(filename));
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

void MainWindow::onFindNext() {
    auto editor = getActiveEditor();
    if (editor) {
        editor->findNext(
            search_dialog_->getSearchText(),
            search_dialog_->isCaseSensitive(),
            search_dialog_->isRegex()
        );
    }
}

void MainWindow::onFindPrevious() {
    auto editor = getActiveEditor();
    if (editor) {
        editor->findPrevious(
            search_dialog_->getSearchText(),
            search_dialog_->isCaseSensitive(),
            search_dialog_->isRegex()
        );
    }
}

void MainWindow::onReplace() {
    auto editor = getActiveEditor();
    if (editor) {
        editor->replace(
            search_dialog_->getSearchText(),
            search_dialog_->getReplaceText(),
            search_dialog_->isCaseSensitive(),
            search_dialog_->isRegex()
        );
    }
}

void MainWindow::onReplaceAll() {
    auto editor = getActiveEditor();
    if (editor) {
        editor->replaceAll(
            search_dialog_->getSearchText(),
            search_dialog_->getReplaceText(),
            search_dialog_->isCaseSensitive(),
            search_dialog_->isRegex()
        );
    }
}


void MainWindow::onQuickOpen() {
    quick_open_dialog_->setWorkingDirectory(working_directory_);
    quick_open_dialog_->show();
    if (quick_open_dialog_->run() == Gtk::RESPONSE_OK) {
        std::string filepath = quick_open_dialog_->getSelectedFile();
        if (!filepath.empty()) {
            try {
                std::string content = xenon::core::FileManager::readFile(filepath);
                createNewTab();

                EditorWidget* editor = getActiveEditor();
                if (editor) {
                    editor->setContent(content);
                    editor->setFilePath(filepath);
                    int pageNum = notebook_.get_current_page();
                    notebook_.set_tab_label_text(
                        *notebook_.get_nth_page(pageNum),
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

void MainWindow::onSplitHorizontal() {
    auto split_pane = getCurrentSplitPane();
    if (split_pane) {
        split_pane->splitHorizontal();
    }
}

void MainWindow::onSplitVertical() {
    auto split_pane = getCurrentSplitPane();
    if (split_pane) {
        split_pane->splitVertical();
    }
}

void MainWindow::onSelectLanguage() {
    EditorWidget* editor = getActiveEditor();
    if (!editor) {
        return;
    }

    std::vector<std::pair<std::string, std::string>> languages = {
        {"C++", "cpp"},
        {"Python", "python"},
        {"JavaScript", "js"},
        {"Java", "java"},
        {"C", "c"},
        {"Go", "go"},
        {"Rust", "rust"},
        {"Ruby", "ruby"},
        {"PHP", "php"},
        {"C#", "csharp"},
        {"JSON", "json"},
        {"XML", "xml"},
        {"HTML", "html"},
    };

    Gtk::Dialog dialog("Select Language", *this, true);
    dialog.set_default_size(300, 400);

    auto contentArea = dialog.get_content_area();
    Gtk::ScrolledWindow scrolled;
    scrolled.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    Gtk::ListBox listBox;
    scrolled.add(listBox);

    for (const auto& [name, lang] : languages) {
        auto label = Gtk::manage(new Gtk::Label(name));
        label->set_halign(Gtk::ALIGN_START);
        label->set_margin_start(12);
        label->set_margin_top(8);
        label->set_margin_bottom(8);
        listBox.append(*label);
    }

    contentArea->pack_start(scrolled, true, true);
    contentArea->show_all();

    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);

    int row = 0;
    listBox.signal_row_selected().connect([&](Gtk::ListBoxRow* selected_row) {
        if (selected_row) {
            row = selected_row->get_index();
        }
    });

    if (dialog.run() == Gtk::RESPONSE_OK && row >= 0 && row < static_cast<int>(languages.size())) {
        editor->setLanguage(languages[row].second);
    }
}

} // namespace xenon::ui
