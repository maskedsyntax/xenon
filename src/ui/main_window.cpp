#include "ui/main_window.hpp"
#include "ui/theme_manager.hpp"
#include "core/file_manager.hpp"
#include <filesystem>

namespace fs = std::filesystem;

namespace xenon::ui {

MainWindow::MainWindow(Glib::RefPtr<Gtk::Application> app)
    : Gtk::ApplicationWindow(app), app_(app) {
    set_title("Xenon");
    set_default_size(1200, 800);

    working_directory_ = fs::current_path().string();

    // Apply dark theme
    ThemeManager::instance().applyDarkTheme(get_screen());

    setupUI();
    setupMenuBar();
    createNewTab();
    setupCommands();

    show_all_children();

    if (terminal_widget_) {
        terminal_widget_->set_no_show_all(true);
        terminal_widget_->hide();
    }
}

void MainWindow::setupUI() {
    main_box_.set_margin_top(0);
    main_box_.set_margin_bottom(0);
    main_box_.set_margin_start(0);
    main_box_.set_margin_end(0);

    accel_group_ = Gtk::AccelGroup::create();
    add_accel_group(accel_group_);

    main_box_.pack_start(menubar_, false, false);

    search_dialog_ = std::make_unique<SearchReplaceDialog>(*this);
    search_dialog_->signal_find_next().connect(sigc::mem_fun(*this, &MainWindow::onFindNext));
    search_dialog_->signal_find_previous().connect(sigc::mem_fun(*this, &MainWindow::onFindPrevious));
    search_dialog_->signal_replace().connect(sigc::mem_fun(*this, &MainWindow::onReplace));
    search_dialog_->signal_replace_all().connect(sigc::mem_fun(*this, &MainWindow::onReplaceAll));

    quick_open_dialog_ = std::make_unique<QuickOpenDialog>(*this);
    quick_open_dialog_->setWorkingDirectory(working_directory_);

    file_explorer_ = std::make_unique<FileExplorer>();
    file_explorer_->set_size_request(220, -1);
    file_explorer_->signal_file_activated().connect(
        sigc::mem_fun(*this, &MainWindow::onExplorerFileActivated));
    file_explorer_->setRootDirectory(working_directory_);

    terminal_widget_ = std::make_unique<TerminalWidget>();
    terminal_widget_->setWorkingDirectory(working_directory_);
    terminal_widget_->get_style_context()->add_class("xenon-terminal");

    command_palette_ = std::make_unique<CommandPalette>(*this);

    content_vpaned_.pack1(notebook_, true, true);
    content_vpaned_.pack2(*terminal_widget_, false, true);
    content_vpaned_.set_position(600);

    main_paned_.pack1(*file_explorer_, false, false);
    content_box_.pack_start(content_vpaned_, true, true);
    main_paned_.pack2(content_box_, true, true);
    main_paned_.set_position(220);

    main_box_.pack_start(main_paned_, true, true);
    main_box_.pack_end(status_bar_, false, false);

    // Update status bar on tab switch
    notebook_.signal_switch_page().connect([this](Gtk::Widget*, guint) {
        updateStatusBar();
    });

    add(main_box_);
}

void MainWindow::setupMenuBar() {
    // ---- File menu ----
    auto* fileMenu = Gtk::manage(new Gtk::Menu());
    fileMenu->set_accel_group(accel_group_);

    auto addItem = [&](Gtk::Menu* menu, const char* label, sigc::slot<void> slot,
                       guint key = 0, Gdk::ModifierType mod = Gdk::ModifierType(0)) {
        auto* item = Gtk::manage(new Gtk::MenuItem(label, true));
        item->signal_activate().connect(slot);
        if (key) {
            item->add_accelerator("activate", accel_group_, key, mod, Gtk::ACCEL_VISIBLE);
        }
        menu->append(*item);
        return item;
    };

    addItem(fileMenu, "_New", sigc::mem_fun(*this, &MainWindow::onFileNew),
            GDK_KEY_n, Gdk::CONTROL_MASK);
    addItem(fileMenu, "_Open File", sigc::mem_fun(*this, &MainWindow::onFileOpen),
            GDK_KEY_o, Gdk::CONTROL_MASK);
    addItem(fileMenu, "Open _Folder", sigc::mem_fun(*this, &MainWindow::onOpenFolder));
    fileMenu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
    addItem(fileMenu, "_Save", sigc::mem_fun(*this, &MainWindow::onFileSave),
            GDK_KEY_s, Gdk::CONTROL_MASK);
    addItem(fileMenu, "Save _As", sigc::mem_fun(*this, &MainWindow::onFileSaveAs));
    fileMenu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
    addItem(fileMenu, "Close _Tab", sigc::mem_fun(*this, &MainWindow::onFileCloseTab),
            GDK_KEY_w, Gdk::CONTROL_MASK);
    addItem(fileMenu, "_Quit", sigc::mem_fun(*this, &MainWindow::onFileQuit));

    auto* fileMenuitem = Gtk::manage(new Gtk::MenuItem("_File", true));
    fileMenuitem->set_submenu(*fileMenu);

    // ---- Edit menu ----
    auto* editMenu = Gtk::manage(new Gtk::Menu());
    editMenu->set_accel_group(accel_group_);

    addItem(editMenu, "_Command Palette", sigc::mem_fun(*this, &MainWindow::onCommandPalette),
            GDK_KEY_p, static_cast<Gdk::ModifierType>(Gdk::CONTROL_MASK | Gdk::SHIFT_MASK));
    addItem(editMenu, "Quick _Open", sigc::mem_fun(*this, &MainWindow::onQuickOpen),
            GDK_KEY_p, Gdk::CONTROL_MASK);
    editMenu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
    addItem(editMenu, "_Find", sigc::mem_fun(*this, &MainWindow::onEditFind),
            GDK_KEY_f, Gdk::CONTROL_MASK);
    addItem(editMenu, "Find and _Replace", sigc::mem_fun(*this, &MainWindow::onEditFindReplace),
            GDK_KEY_h, Gdk::CONTROL_MASK);

    auto* editMenuitem = Gtk::manage(new Gtk::MenuItem("_Edit", true));
    editMenuitem->set_submenu(*editMenu);

    // ---- View menu ----
    auto* viewMenu = Gtk::manage(new Gtk::Menu());
    viewMenu->set_accel_group(accel_group_);

    addItem(viewMenu, "Toggle _Sidebar", sigc::mem_fun(*this, &MainWindow::onToggleSidebar),
            GDK_KEY_b, Gdk::CONTROL_MASK);
    addItem(viewMenu, "Toggle _Minimap", sigc::mem_fun(*this, &MainWindow::onToggleMinimap));
    addItem(viewMenu, "Toggle _Terminal", sigc::mem_fun(*this, &MainWindow::onToggleTerminal),
            GDK_KEY_grave, Gdk::CONTROL_MASK);
    viewMenu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
    addItem(viewMenu, "Split _Horizontally", sigc::mem_fun(*this, &MainWindow::onSplitHorizontal),
            GDK_KEY_h, Gdk::MOD1_MASK);
    addItem(viewMenu, "Split _Vertically", sigc::mem_fun(*this, &MainWindow::onSplitVertical),
            GDK_KEY_v, Gdk::MOD1_MASK);
    viewMenu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
    addItem(viewMenu, "Set _Language", sigc::mem_fun(*this, &MainWindow::onSelectLanguage));

    auto* viewMenuitem = Gtk::manage(new Gtk::MenuItem("_View", true));
    viewMenuitem->set_submenu(*viewMenu);

    menubar_.append(*fileMenuitem);
    menubar_.append(*editMenuitem);
    menubar_.append(*viewMenuitem);
    menubar_.show_all();
}

void MainWindow::setupCommands() {
    command_palette_->clearCommands();

    auto add = [this](const char* name, const char* shortcut, std::function<void()> fn) {
        command_palette_->addCommand(name, shortcut, std::move(fn));
    };

    add("New File",              "Ctrl+N",       [this]{ onFileNew(); });
    add("Open File",             "Ctrl+O",       [this]{ onFileOpen(); });
    add("Open Folder",           "",             [this]{ onOpenFolder(); });
    add("Save",                  "Ctrl+S",       [this]{ onFileSave(); });
    add("Save As",               "",             [this]{ onFileSaveAs(); });
    add("Close Tab",             "Ctrl+W",       [this]{ onFileCloseTab(); });
    add("Quit",                  "",             [this]{ onFileQuit(); });
    add("Find",                  "Ctrl+F",       [this]{ onEditFind(); });
    add("Find and Replace",      "Ctrl+H",       [this]{ onEditFindReplace(); });
    add("Quick Open",            "Ctrl+P",       [this]{ onQuickOpen(); });
    add("Toggle Terminal",       "Ctrl+`",       [this]{ onToggleTerminal(); });
    add("Toggle Minimap",        "",             [this]{ onToggleMinimap(); });
    add("Toggle Sidebar",        "Ctrl+B",       [this]{ onToggleSidebar(); });
    add("Split Horizontally",    "Alt+H",        [this]{ onSplitHorizontal(); });
    add("Split Vertically",      "Alt+V",        [this]{ onSplitVertical(); });
    add("Set Language",          "",             [this]{ onSelectLanguage(); });
}

void MainWindow::connectEditorSignals(EditorWidget* editor) {
    if (!editor) return;
    editor->signal_cursor_moved().connect([this](int line, int col) {
        status_bar_.setCursorPosition(line, col);
    });
    editor->signal_content_changed().connect([this]() {
        // Could update modified indicator in tab title here
    });
}

void MainWindow::createNewTab() {
    auto* split_pane = Gtk::manage(new SplitPaneContainer());
    int pageNum = notebook_.append_page(*split_pane);
    auto* tabLabel = createTabLabel("Untitled", split_pane);
    notebook_.set_tab_label(*split_pane, *tabLabel);
    notebook_.set_tab_reorderable(*split_pane, true);

    split_pane->show_all();
    notebook_.set_current_page(pageNum);

    Glib::signal_idle().connect([this, split_pane]() {
        auto* editor = split_pane->getActiveEditor();
        if (editor) {
            connectEditorSignals(editor);
            editor->grab_focus();
        }
        return false;
    });
}

Gtk::Widget* MainWindow::createTabLabel(const std::string& title, Gtk::Widget* page) {
    auto* box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
    auto* label = Gtk::manage(new Gtk::Label(title));
    auto* closeBtn = Gtk::manage(new Gtk::Button());

    auto* image = Gtk::manage(new Gtk::Image(Gtk::Stock::CLOSE, Gtk::ICON_SIZE_MENU));
    closeBtn->set_image(*image);
    closeBtn->set_relief(Gtk::RELIEF_NONE);
    closeBtn->set_focus_on_click(false);

    closeBtn->signal_clicked().connect(
        sigc::bind(sigc::mem_fun(*this, &MainWindow::closeTab), page));

    box->pack_start(*label, true, true);
    box->pack_start(*closeBtn, false, false);
    box->show_all();

    return box;
}

void MainWindow::closeTab(Gtk::Widget* page) {
    int pageNum = notebook_.page_num(*page);
    if (pageNum != -1) {
        notebook_.remove_page(pageNum);
    }
    if (notebook_.get_n_pages() == 0) {
        createNewTab();
    }
}

SplitPaneContainer* MainWindow::getCurrentSplitPane() {
    int pageNum = notebook_.get_current_page();
    if (pageNum >= 0) {
        return dynamic_cast<SplitPaneContainer*>(notebook_.get_nth_page(pageNum));
    }
    return nullptr;
}

EditorWidget* MainWindow::getActiveEditor() {
    auto* split_pane = getCurrentSplitPane();
    if (split_pane) {
        return split_pane->getActiveEditor();
    }
    return nullptr;
}

void MainWindow::updateStatusBar() {
    auto* editor = getActiveEditor();
    if (!editor) {
        status_bar_.setCursorPosition(1, 1);
        status_bar_.setLanguage("Plain Text");
        status_bar_.setEncoding("UTF-8");
        status_bar_.setLineEnding("LF");
        return;
    }
    auto [line, col] = editor->getCursorPosition();
    status_bar_.setCursorPosition(line, col);
    status_bar_.setLanguage(editor->getLanguageName());
    status_bar_.setEncoding(editor->getEncoding());
    status_bar_.setLineEnding(editor->getLineEnding());
}

void MainWindow::updateTabLabel(const std::string& title) {
    int pageNum = notebook_.get_current_page();
    if (pageNum < 0) return;
    auto* page = notebook_.get_nth_page(pageNum);
    if (!page) return;
    auto* tabWidget = notebook_.get_tab_label(*page);
    if (auto* box = dynamic_cast<Gtk::Container*>(tabWidget)) {
        auto children = box->get_children();
        if (!children.empty()) {
            if (auto* label = dynamic_cast<Gtk::Label*>(children[0])) {
                label->set_text(title);
            }
        }
    }
}

// ---- File actions ----

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
            auto* editor = getActiveEditor();
            if (editor) {
                editor->setContent(content);
                editor->setFilePath(filename);
                updateTabLabel(xenon::core::FileManager::getFileName(filename));
                updateStatusBar();
            }
        } catch (const std::exception& e) {
            Gtk::MessageDialog err(*this, e.what(), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
            err.run();
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
        set_title("Xenon - " + folder);
        quick_open_dialog_->setWorkingDirectory(folder);
        file_explorer_->setRootDirectory(folder);
        if (terminal_widget_) {
            terminal_widget_->setWorkingDirectory(folder);
        }
    }
}

void MainWindow::onFileSave() {
    auto* editor = getActiveEditor();
    if (editor) {
        if (editor->getFilePath().empty()) {
            onFileSaveAs();
        } else {
            editor->saveFile();
            status_bar_.setMessage("Saved");
            // Clear message after 2 seconds
            Glib::signal_timeout().connect_once([this]() {
                status_bar_.clearMessage();
            }, 2000);
        }
    }
}

void MainWindow::onFileSaveAs() {
    auto* editor = getActiveEditor();
    if (!editor) return;

    Gtk::FileChooserDialog dialog("Save File As", Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.set_transient_for(*this);
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);

    if (dialog.run() == Gtk::RESPONSE_OK) {
        std::string filename = dialog.get_filename();
        editor->setFilePath(filename);
        editor->saveFile();
        updateTabLabel(xenon::core::FileManager::getFileName(filename));
        updateStatusBar();
    }
}

void MainWindow::onFileCloseTab() {
    int pageNum = notebook_.get_current_page();
    if (pageNum >= 0) {
        closeTab(notebook_.get_nth_page(pageNum));
    }
}

void MainWindow::onFileQuit() {
    close();
}

bool MainWindow::on_delete_event(GdkEventAny* /* any_event */) {
    app_->quit();
    return true;
}

// ---- Edit actions ----

void MainWindow::onEditFind() {
    search_dialog_->showSearch();
    search_dialog_->show();
}

void MainWindow::onEditFindReplace() {
    search_dialog_->showSearchReplace();
    search_dialog_->show();
}

void MainWindow::onFindNext() {
    auto* editor = getActiveEditor();
    if (editor) {
        editor->findNext(search_dialog_->getSearchText(),
                         search_dialog_->isCaseSensitive(),
                         search_dialog_->isRegex());
    }
}

void MainWindow::onFindPrevious() {
    auto* editor = getActiveEditor();
    if (editor) {
        editor->findPrevious(search_dialog_->getSearchText(),
                             search_dialog_->isCaseSensitive(),
                             search_dialog_->isRegex());
    }
}

void MainWindow::onReplace() {
    auto* editor = getActiveEditor();
    if (editor) {
        editor->replace(search_dialog_->getSearchText(),
                        search_dialog_->getReplaceText(),
                        search_dialog_->isCaseSensitive(),
                        search_dialog_->isRegex());
    }
}

void MainWindow::onReplaceAll() {
    auto* editor = getActiveEditor();
    if (editor) {
        editor->replaceAll(search_dialog_->getSearchText(),
                           search_dialog_->getReplaceText(),
                           search_dialog_->isCaseSensitive(),
                           search_dialog_->isRegex());
    }
}

void MainWindow::onCommandPalette() {
    command_palette_->show();
    command_palette_->run();
    command_palette_->hide();
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
                auto* editor = getActiveEditor();
                if (editor) {
                    editor->setContent(content);
                    editor->setFilePath(filepath);
                    updateTabLabel(xenon::core::FileManager::getFileName(filepath));
                    updateStatusBar();
                }
            } catch (const std::exception& e) {
                Gtk::MessageDialog err(*this, e.what(), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
                err.run();
            }
        }
    }
    quick_open_dialog_->hide();
}

// ---- View actions ----

void MainWindow::onToggleSidebar() {
    if (file_explorer_->is_visible()) {
        file_explorer_->hide();
    } else {
        file_explorer_->show();
    }
}

void MainWindow::onToggleMinimap() {
    auto* editor = getActiveEditor();
    if (editor) {
        editor->toggleMinimap();
    }
}

void MainWindow::onSplitHorizontal() {
    auto* split_pane = getCurrentSplitPane();
    if (split_pane) {
        split_pane->splitHorizontal();
    }
}

void MainWindow::onSplitVertical() {
    auto* split_pane = getCurrentSplitPane();
    if (split_pane) {
        split_pane->splitVertical();
    }
}

void MainWindow::onToggleTerminal() {
    if (terminal_widget_) {
        terminal_widget_->toggle();
    }
}

void MainWindow::onSelectLanguage() {
    auto* editor = getActiveEditor();
    if (!editor) return;

    std::vector<std::pair<std::string, std::string>> languages = {
        {"C",          "c"},
        {"C++",        "cpp"},
        {"C#",         "csharp"},
        {"Go",         "go"},
        {"HTML",       "html"},
        {"Java",       "java"},
        {"JavaScript", "js"},
        {"JSON",       "json"},
        {"Markdown",   "markdown"},
        {"PHP",        "php"},
        {"Python",     "python"},
        {"Ruby",       "ruby"},
        {"Rust",       "rust"},
        {"Shell",      "sh"},
        {"SQL",        "sql"},
        {"TypeScript", "typescript"},
        {"XML",        "xml"},
        {"YAML",       "yaml"},
    };

    Gtk::Dialog dialog("Set Language", *this, true);
    dialog.set_default_size(300, 420);

    auto* contentArea = dialog.get_content_area();

    Gtk::Entry filter_entry;
    filter_entry.set_placeholder_text("Filter languages...");
    filter_entry.set_margin_start(8);
    filter_entry.set_margin_end(8);
    filter_entry.set_margin_top(8);
    filter_entry.set_margin_bottom(4);
    contentArea->pack_start(filter_entry, false, false);

    Gtk::ScrolledWindow scrolled;
    scrolled.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    Gtk::ListBox listBox;
    scrolled.add(listBox);
    scrolled.set_min_content_height(320);

    auto rebuildList = [&]() {
        for (auto* child : listBox.get_children()) {
            listBox.remove(*child);
        }
        std::string filter = filter_entry.get_text().raw();
        std::transform(filter.begin(), filter.end(), filter.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        for (const auto& [name, lang] : languages) {
            std::string lower_name = name;
            std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (filter.empty() || lower_name.find(filter) != std::string::npos) {
                auto* label = Gtk::manage(new Gtk::Label(name));
                label->set_halign(Gtk::ALIGN_START);
                label->set_margin_start(12);
                label->set_margin_top(8);
                label->set_margin_bottom(8);
                listBox.append(*label);
            }
        }
        listBox.show_all();
        auto* first = listBox.get_row_at_index(0);
        if (first) listBox.select_row(*first);
    };

    rebuildList();
    filter_entry.signal_changed().connect(rebuildList);

    contentArea->pack_start(scrolled, true, true);
    contentArea->show_all();

    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);

    // Double-click activates
    listBox.signal_row_activated().connect([&](Gtk::ListBoxRow*) {
        dialog.response(Gtk::RESPONSE_OK);
    });

    if (dialog.run() == Gtk::RESPONSE_OK) {
        auto* selected = listBox.get_selected_row();
        if (selected) {
            int idx = selected->get_index();
            // Map index back to filtered list
            std::string filter = filter_entry.get_text().raw();
            std::transform(filter.begin(), filter.end(), filter.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            int count = 0;
            for (const auto& [name, lang] : languages) {
                std::string lower_name = name;
                std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                if (filter.empty() || lower_name.find(filter) != std::string::npos) {
                    if (count == idx) {
                        editor->setLanguage(lang);
                        updateStatusBar();
                        break;
                    }
                    count++;
                }
            }
        }
    }
}

void MainWindow::onExplorerFileActivated(const std::string& path) {
    if (path.empty()) return;

    try {
        std::string content = xenon::core::FileManager::readFile(path);
        std::string filename = xenon::core::FileManager::getFileName(path);

        auto* editor = getActiveEditor();
        bool reuseTab = editor && !editor->isModified() &&
                        editor->getFilePath().empty() && editor->getContent().empty();

        if (!reuseTab) {
            createNewTab();
            editor = getActiveEditor();
        }

        if (editor) {
            editor->setContent(content);
            editor->setFilePath(path);
            updateTabLabel(filename);
            updateStatusBar();
        }
    } catch (const std::exception& e) {
        Gtk::MessageDialog err(*this, e.what(), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
        err.run();
    }
}

} // namespace xenon::ui
