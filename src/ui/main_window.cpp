#include "ui/main_window.hpp"
#include "ui/theme_manager.hpp"
#include "core/file_manager.hpp"
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <array>

namespace fs = std::filesystem;

namespace xenon::ui {

MainWindow::MainWindow(Glib::RefPtr<Gtk::Application> app)
    : Gtk::ApplicationWindow(app), app_(app) {
    set_title("Xenon");
    set_default_size(1200, 800);

    working_directory_ = fs::current_path().string();

    // Apply dark theme
    ThemeManager::instance().applyDarkTheme(get_screen());

    // Initialize git manager
    git_manager_ = std::make_shared<xenon::git::GitManager>();
    if (git_manager_->setWorkingDirectory(working_directory_)) {
        // Will show branch in status bar after UI is set up
    }

    setupUI();
    setupMenuBar();
    createNewTab();
    setupCommands();

    show_all_children();

    if (terminal_widget_) {
        terminal_widget_->set_no_show_all(true);
        terminal_widget_->hide();
    }

    // Update git branch in status bar
    if (git_manager_ && git_manager_->isGitRepo()) {
        status_bar_.setGitBranch(git_manager_->currentBranch());
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
    file_explorer_->signal_file_activated().connect(
        sigc::mem_fun(*this, &MainWindow::onExplorerFileActivated));
    file_explorer_->setRootDirectory(working_directory_);

    search_panel_ = std::make_unique<SearchPanel>();
    search_panel_->setWorkingDirectory(working_directory_);
    search_panel_->setFileOpenCallback(
        sigc::mem_fun(*this, &MainWindow::openFileAtLine));

    problems_panel_ = std::make_unique<ProblemsPanel>();
    problems_panel_->setJumpCallback([this](const std::string& uri, int line, int col) {
        std::string path = uri;
        if (path.substr(0, 7) == "file://") path = path.substr(7);
        openFileAtLine(path, line, col);
    });

    settings_dialog_ = std::make_unique<SettingsDialog>(*this);
    settings_dialog_->setApplyCallback([this](const EditorSettings& s) {
        current_settings_ = s;
        applySettingsToAllEditors();
    });

    terminal_widget_ = std::make_unique<TerminalWidget>();
    terminal_widget_->setWorkingDirectory(working_directory_);
    terminal_widget_->get_style_context()->add_class("xenon-terminal");

    command_palette_ = std::make_unique<CommandPalette>(*this);

    // Breadcrumb bar above the editor notebook
    auto* editor_area = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    breadcrumb_bar_.set_size_request(-1, 28);
    breadcrumb_bar_.setDirCallback([this](const std::string& path) {
        // Open folder in explorer when breadcrumb segment is clicked
        file_explorer_->setRootDirectory(path);
    });
    editor_area->pack_start(breadcrumb_bar_, false, false);
    editor_area->pack_start(notebook_, true, true);

    content_vpaned_.pack1(*editor_area, true, true);
    content_vpaned_.pack2(*terminal_widget_, false, true);
    content_vpaned_.set_position(600);

    // Sidebar: tabbed notebook with Explorer, Search, and Problems panels
    sidebar_notebook_.set_tab_pos(Gtk::POS_TOP);
    sidebar_notebook_.set_size_request(240, -1);
    sidebar_notebook_.append_page(*file_explorer_, "Explorer");
    sidebar_notebook_.append_page(*search_panel_, "Search");
    sidebar_notebook_.append_page(*problems_panel_, "Problems");

    main_paned_.pack1(sidebar_notebook_, false, false);
    content_box_.pack_start(content_vpaned_, true, true);
    main_paned_.pack2(content_box_, true, true);
    main_paned_.set_position(240);

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
    fileMenu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

    // Recent files submenu
    recent_menu_ = Gtk::manage(new Gtk::Menu());
    auto* recentItem = Gtk::manage(new Gtk::MenuItem("Recent _Files", true));
    recentItem->set_submenu(*recent_menu_);
    fileMenu->append(*recentItem);
    rebuildRecentFilesMenu();
    fileMenu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

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
    editMenu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
    addItem(editMenu, "_Go to Definition", sigc::mem_fun(*this, &MainWindow::onGotoDefinition),
            GDK_KEY_F12, Gdk::ModifierType(0));
    addItem(editMenu, "Trigger _Completion", sigc::mem_fun(*this, &MainWindow::onTriggerCompletion),
            GDK_KEY_space, Gdk::CONTROL_MASK);
    addItem(editMenu, "_Global Search", sigc::mem_fun(*this, &MainWindow::onGlobalSearch),
            GDK_KEY_f, static_cast<Gdk::ModifierType>(Gdk::CONTROL_MASK | Gdk::SHIFT_MASK));
    editMenu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
    addItem(editMenu, "_Preferences", sigc::mem_fun(*this, &MainWindow::onPreferences),
            GDK_KEY_comma, Gdk::CONTROL_MASK);
    editMenu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
    addItem(editMenu, "_Undo", sigc::mem_fun(*this, &MainWindow::onUndo),
            GDK_KEY_z, Gdk::CONTROL_MASK);
    addItem(editMenu, "_Redo", sigc::mem_fun(*this, &MainWindow::onRedo),
            GDK_KEY_y, Gdk::CONTROL_MASK);
    editMenu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
    addItem(editMenu, "Toggle _Line Comment", sigc::mem_fun(*this, &MainWindow::onToggleLineComment),
            GDK_KEY_slash, Gdk::CONTROL_MASK);
    addItem(editMenu, "Toggle _Block Comment", sigc::mem_fun(*this, &MainWindow::onToggleBlockComment),
            GDK_KEY_slash, static_cast<Gdk::ModifierType>(Gdk::CONTROL_MASK | Gdk::SHIFT_MASK));

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
    viewMenu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
    addItem(viewMenu, "Zoom _In",  sigc::mem_fun(*this, &MainWindow::onZoomIn),
            GDK_KEY_equal, Gdk::CONTROL_MASK);
    addItem(viewMenu, "Zoom _Out", sigc::mem_fun(*this, &MainWindow::onZoomOut),
            GDK_KEY_minus, Gdk::CONTROL_MASK);
    addItem(viewMenu, "Reset _Zoom", sigc::mem_fun(*this, &MainWindow::onZoomReset),
            GDK_KEY_0, Gdk::CONTROL_MASK);
    viewMenu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
    addItem(viewMenu, "_Zen Mode", sigc::mem_fun(*this, &MainWindow::onToggleZenMode),
            GDK_KEY_F11, Gdk::ModifierType(0));

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
    add("Go to Definition",      "F12",          [this]{ onGotoDefinition(); });
    add("Trigger Completion",    "Ctrl+Space",   [this]{ onTriggerCompletion(); });
    add("Global Search",         "Ctrl+Shift+F", [this]{ onGlobalSearch(); });
    add("Preferences",           "Ctrl+,",       [this]{ onPreferences(); });
}

// ---- LSP management ----

std::shared_ptr<xenon::lsp::LspClient> MainWindow::getLspClientForEditor(EditorWidget* editor) {
    if (!editor) return nullptr;
    std::string path = editor->getFilePath();
    if (path.empty()) return nullptr;

    // Determine which language server to use based on file extension
    std::string ext = fs::path(path).extension().string();
    std::string server_key;
    std::vector<std::string> cmd;

    if (ext == ".cpp" || ext == ".cxx" || ext == ".cc" || ext == ".c" ||
        ext == ".h" || ext == ".hpp" || ext == ".hxx") {
        server_key = "clangd";
        cmd = {"clangd", "--background-index", "--clang-tidy"};
    } else if (ext == ".rs") {
        server_key = "rust-analyzer";
        cmd = {"rust-analyzer"};
    } else if (ext == ".go") {
        server_key = "gopls";
        cmd = {"gopls"};
    } else if (ext == ".py") {
        server_key = "pylsp";
        cmd = {"pylsp"};
    } else {
        return nullptr;
    }

    auto it = lsp_clients_.find(server_key);
    if (it != lsp_clients_.end() && it->second->isRunning()) {
        return it->second;
    }

    // Start a new LSP server
    auto client = std::make_shared<xenon::lsp::LspClient>();
    std::string rootUri = "file://" + working_directory_;
    if (client->start(cmd, rootUri)) {
        lsp_clients_[server_key] = client;
        return client;
    }
    return nullptr;
}

void MainWindow::connectEditorSignals(EditorWidget* editor) {
    if (!editor) return;
    editor->signal_cursor_moved().connect([this](int line, int col) {
        status_bar_.setCursorPosition(line, col);
    });
    editor->signal_content_changed().connect([this]() {
        // Mark tab as modified if the active editor was changed
        auto* active = getActiveEditor();
        if (active && active->isModified()) {
            markTabModified(true);
        }
    });

    // Attach LSP client if a suitable server is available
    auto lsp = getLspClientForEditor(editor);
    if (lsp) {
        // Hook diagnostics into the problems panel
        std::string ep = editor->getFilePath();
        lsp->setDiagnosticsCallback([this, ep](const std::string& uri,
                                               std::vector<xenon::lsp::Diagnostic> diags) {
            // Forward to problems panel on main thread
            Glib::signal_idle().connect_once(
                [this, uri, d = std::move(diags)]() {
                    if (problems_panel_) {
                        problems_panel_->updateDiagnostics(uri, d);
                    }
                });
        });
        editor->setLspClient(lsp);
    }

    // Attach git manager for diff gutter marks
    if (git_manager_ && git_manager_->isGitRepo()) {
        editor->setGitManager(git_manager_);
    }
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
        breadcrumb_bar_.setPath("");
        return;
    }
    auto [line, col] = editor->getCursorPosition();
    status_bar_.setCursorPosition(line, col);
    status_bar_.setLanguage(editor->getLanguageName());
    status_bar_.setEncoding(editor->getEncoding());
    status_bar_.setLineEnding(editor->getLineEnding());
    breadcrumb_bar_.setPath(editor->getFilePath());
}

void MainWindow::markTabModified(bool modified) {
    int pageNum = notebook_.get_current_page();
    if (pageNum < 0) return;
    auto* page = notebook_.get_nth_page(pageNum);
    if (!page) return;
    auto* tabWidget = notebook_.get_tab_label(*page);
    if (auto* box = dynamic_cast<Gtk::Container*>(tabWidget)) {
        auto children = box->get_children();
        if (!children.empty()) {
            if (auto* label = dynamic_cast<Gtk::Label*>(children[0])) {
                std::string text = label->get_text();
                bool hasPrefix = !text.empty() && text[0] == '\u25CF';  // ● U+25CF
                if (modified && !hasPrefix) {
                    label->set_text("\u25CF " + text);
                } else if (!modified && hasPrefix) {
                    label->set_text(text.substr(3));  // Remove "● " (3 bytes in UTF-8)
                }
            }
        }
    }
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
                auto lsp = getLspClientForEditor(editor);
                if (lsp) editor->setLspClient(lsp);
                updateTabLabel(xenon::core::FileManager::getFileName(filename));
                updateStatusBar();
                addToRecentFiles(filename);
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
        search_panel_->setWorkingDirectory(folder);
        if (terminal_widget_) {
            terminal_widget_->setWorkingDirectory(folder);
        }
        if (git_manager_) {
            if (git_manager_->setWorkingDirectory(folder)) {
                status_bar_.setGitBranch(git_manager_->currentBranch());
            } else {
                status_bar_.setGitBranch("");
            }
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
            markTabModified(false);
            status_bar_.setMessage("Saved");
            Glib::signal_timeout().connect_once([this]() {
                status_bar_.clearMessage();
            }, 2000);
            // Refresh git diff marks after save
            if (git_manager_) editor->refreshGitDiff();
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
                    addToRecentFiles(filepath);
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
    if (sidebar_notebook_.is_visible()) {
        sidebar_notebook_.hide();
    } else {
        sidebar_notebook_.show();
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

void MainWindow::onPreferences() {
    settings_dialog_->setSettings(current_settings_);
    settings_dialog_->show();
    settings_dialog_->run();
    settings_dialog_->hide();
}

void MainWindow::applySettingsToAllEditors() {
    for (int i = 0; i < notebook_.get_n_pages(); ++i) {
        auto* split = dynamic_cast<SplitPaneContainer*>(notebook_.get_nth_page(i));
        if (split) {
            for (auto* ed : split->getAllEditors()) {
                if (ed) ed->applySettings(current_settings_);
            }
        }
    }
}

void MainWindow::onGlobalSearch() {
    // Switch sidebar to the Search tab
    sidebar_notebook_.set_current_page(1);
    search_panel_->focusSearch();
}

void MainWindow::openFileAtLine(const std::string& path, int line, int col) {
    try {
        std::string content = xenon::core::FileManager::readFile(path);

        // Check if already open
        for (int i = 0; i < notebook_.get_n_pages(); ++i) {
            auto* split = dynamic_cast<SplitPaneContainer*>(notebook_.get_nth_page(i));
            if (split) {
                auto* ed = split->getActiveEditor();
                if (ed && ed->getFilePath() == path) {
                    notebook_.set_current_page(i);
                    // Jump to line
                    auto buf = ed->getSourceView()->get_buffer();
                    int max_ln = buf->get_line_count() - 1;
                    auto iter = buf->get_iter_at_line_offset(
                        std::min(line - 1, max_ln),
                        std::max(0, col - 1));
                    buf->place_cursor(iter);
                    ed->getSourceView()->scroll_to(iter, 0.3);
                    return;
                }
            }
        }

        createNewTab();
        auto* editor = getActiveEditor();
        if (editor) {
            editor->setContent(content);
            editor->setFilePath(path);
            auto lsp = getLspClientForEditor(editor);
            if (lsp) editor->setLspClient(lsp);
            if (git_manager_) editor->setGitManager(git_manager_);
            updateTabLabel(xenon::core::FileManager::getFileName(path));
            updateStatusBar();

            // Jump to line
            Glib::signal_idle().connect_once([editor, line, col]() {
                auto buf = editor->getSourceView()->get_buffer();
                int max_ln = buf->get_line_count() - 1;
                auto iter = buf->get_iter_at_line_offset(
                    std::min(line - 1, max_ln),
                    std::max(0, col - 1));
                buf->place_cursor(iter);
                editor->getSourceView()->scroll_to(iter, 0.3);
            });
        }
    } catch (const std::exception& e) {
        Gtk::MessageDialog err(*this, e.what(), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
        err.run();
    }
}

void MainWindow::onGotoDefinition() {
    auto* editor = getActiveEditor();
    if (editor) editor->gotoDefinition();
}

void MainWindow::onTriggerCompletion() {
    auto* editor = getActiveEditor();
    if (editor) editor->triggerCompletion();
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
            auto lsp = getLspClientForEditor(editor);
            if (lsp) editor->setLspClient(lsp);
            updateTabLabel(filename);
            updateStatusBar();
            addToRecentFiles(path);
        }
    } catch (const std::exception& e) {
        Gtk::MessageDialog err(*this, e.what(), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
        err.run();
    }
}

// ---- Zoom actions ----

void MainWindow::onZoomIn() {
    auto* editor = getActiveEditor();
    if (editor) editor->zoomIn();
}

void MainWindow::onZoomOut() {
    auto* editor = getActiveEditor();
    if (editor) editor->zoomOut();
}

void MainWindow::onZoomReset() {
    auto* editor = getActiveEditor();
    if (editor) editor->zoomReset();
}

// ---- Undo / Redo ----

void MainWindow::onUndo() {
    auto* editor = getActiveEditor();
    if (editor) editor->undo();
}

void MainWindow::onRedo() {
    auto* editor = getActiveEditor();
    if (editor) editor->redo();
}

// ---- Zen mode ----

void MainWindow::onToggleZenMode() {
    zen_mode_ = !zen_mode_;
    if (zen_mode_) {
        fullscreen();
        menubar_.hide();
        sidebar_notebook_.hide();
        status_bar_.hide();
        breadcrumb_bar_.hide();
        if (terminal_widget_) terminal_widget_->hide();
    } else {
        unfullscreen();
        menubar_.show();
        sidebar_notebook_.show();
        status_bar_.show();
        breadcrumb_bar_.show();
    }
}

// ---- Line / block commenting ----

void MainWindow::onToggleLineComment() {
    auto* editor = getActiveEditor();
    if (editor) editor->toggleLineComment();
}

void MainWindow::onToggleBlockComment() {
    auto* editor = getActiveEditor();
    if (editor) editor->toggleBlockComment();
}

// ---- Recent files ----

static std::string recentFilesPath() {
    const char* home = std::getenv("HOME");
    std::string dir = home ? std::string(home) + "/.config/xenon" : "/tmp";
    fs::create_directories(dir);
    return dir + "/recent_files";
}

void MainWindow::addToRecentFiles(const std::string& path) {
    if (path.empty()) return;

    // Read existing list
    std::vector<std::string> files;
    std::ifstream in(recentFilesPath());
    if (in) {
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty() && line != path) {
                files.push_back(line);
            }
        }
    }

    // Prepend new entry and cap at 10
    files.insert(files.begin(), path);
    if (files.size() > 10) files.resize(10);

    // Write back
    std::ofstream out(recentFilesPath(), std::ios::trunc);
    for (const auto& f : files) {
        out << f << '\n';
    }

    rebuildRecentFilesMenu();
}

void MainWindow::rebuildRecentFilesMenu() {
    if (!recent_menu_) return;

    // Clear existing items
    for (auto* child : recent_menu_->get_children()) {
        recent_menu_->remove(*child);
    }

    std::vector<std::string> files;
    std::ifstream in(recentFilesPath());
    if (in) {
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty()) files.push_back(line);
        }
    }

    if (files.empty()) {
        auto* empty = Gtk::manage(new Gtk::MenuItem("(No recent files)"));
        empty->set_sensitive(false);
        recent_menu_->append(*empty);
    } else {
        for (const auto& fpath : files) {
            std::string label = fs::path(fpath).filename().string() + "  " + fpath;
            auto* item = Gtk::manage(new Gtk::MenuItem(label));
            item->signal_activate().connect([this, fpath]() {
                openFileAtLine(fpath, 1, 1);
            });
            recent_menu_->append(*item);
        }
    }
    recent_menu_->show_all();
}

} // namespace xenon::ui
