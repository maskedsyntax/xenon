#include "ui/main_window.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMenuBar>
#include <QApplication>
#include <QLabel>
#include <QActionGroup>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <unordered_map>

#include <QMessageBox>

#include "features/search_engine.hpp"

namespace xenon::ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    
    setWindowTitle("Xenon");
    resize(1200, 800);

    git_manager_ = std::make_unique<xenon::git::GitManager>(this);
    connect(git_manager_.get(), &xenon::git::GitManager::branchChanged, this, &MainWindow::updateGitBranch);

    lsp_client_ = std::make_unique<xenon::lsp::LspClient>(this);
    // Start clangd by default if available
    lsp_client_->start({"clangd"}, QDir::currentPath());

#ifdef Q_OS_MAC
    // Modern macOS look: unified title and toolbar
    setUnifiedTitleAndToolBarOnMac(true);
#endif

    setupUI();
    setupMenus();

    git_manager_->setWorkingDirectory(QDir::currentPath());
}

void MainWindow::setupUI() {
    auto* central_widget = new QWidget(this);
    auto* main_layout = new QHBoxLayout(central_widget);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);
    setCentralWidget(central_widget);

    setupActivityBar();

    main_splitter_ = new QSplitter(Qt::Horizontal, this);
    main_splitter_->setHandleWidth(1);
    main_splitter_->setStyleSheet("QSplitter::handle { background-color: #3c3c3c; }");

    setupSidebar();

    editor_tabs_ = new QTabWidget(this);
    editor_tabs_->setTabsClosable(true);
    editor_tabs_->setMovable(true);
    editor_tabs_->setDocumentMode(true);
    editor_tabs_->setStyleSheet(
        "QTabWidget::pane { border: none; background-color: #1e1e1e; }"
        "QTabBar::tab { background-color: #2d2d2d; color: #909090; padding: 8px 12px; border-right: 1px solid #3a3a3a; }"
        "QTabBar::tab:selected { background-color: #1e1e1e; color: #ffffff; border-bottom: 2px solid #007acc; }"
    );
    connect(editor_tabs_, &QTabWidget::tabCloseRequested, this, &MainWindow::onEditorTabClosed);

    terminal_widget_ = new TerminalWidget(this);

    content_splitter_ = new QSplitter(Qt::Vertical, this);
    content_splitter_->setHandleWidth(1);
    content_splitter_->setStyleSheet("QSplitter::handle { background-color: #3c3c3c; }");

    auto* editor_container = new QWidget(this);
    auto* editor_layout = new QVBoxLayout(editor_container);
    editor_layout->setContentsMargins(0, 0, 0, 0);
    editor_layout->setSpacing(0);

    find_replace_widget_ = new FindReplaceWidget(this);
    find_replace_widget_->hide();
    connect(find_replace_widget_, &FindReplaceWidget::findNext, this, &MainWindow::onFindNext);
    connect(find_replace_widget_, &FindReplaceWidget::findPrevious, this, &MainWindow::onFindPrevious);
    connect(find_replace_widget_, &FindReplaceWidget::replace, this, &MainWindow::onReplace);
    connect(find_replace_widget_, &FindReplaceWidget::replaceAll, this, &MainWindow::onReplaceAll);
    connect(find_replace_widget_, &FindReplaceWidget::closeRequested, find_replace_widget_, &FindReplaceWidget::hide);

    editor_layout->addWidget(find_replace_widget_);
    editor_layout->addWidget(editor_tabs_);

    content_splitter_->addWidget(editor_container);
    content_splitter_->addWidget(terminal_widget_);
    content_splitter_->setStretchFactor(0, 1);
    content_splitter_->setStretchFactor(1, 0);
    content_splitter_->setSizes({600, 200});

    main_splitter_->addWidget(sidebar_stack_);
    main_splitter_->addWidget(content_splitter_);
    main_splitter_->setStretchFactor(1, 1);

    main_layout->addWidget(activity_bar_);
    main_layout->addWidget(main_splitter_);

    command_palette_ = new CommandPalette(this);
    quick_open_dialog_ = new QuickOpenDialog(this);
    connect(quick_open_dialog_, &QuickOpenDialog::fileSelected, this, &MainWindow::onFileOpen);

    completion_widget_ = new CompletionWidget(this);
    connect(lsp_client_.get(), &xenon::lsp::LspClient::completionReceived, this, &MainWindow::onCompletionReceived);
    connect(lsp_client_.get(), &xenon::lsp::LspClient::definitionReceived, this, &MainWindow::onDefinitionReceived);
    connect(completion_widget_, &CompletionWidget::completionSelected, this, &MainWindow::onCompletionSelected);

    branch_label_ = new QLabel(this);
    branch_label_->setStyleSheet("padding-left: 10px; color: white;");
    statusBar()->addPermanentWidget(branch_label_);
    statusBar()->setStyleSheet("QStatusBar { background-color: #007acc; color: white; }");
    statusBar()->showMessage("Ready");
}

void MainWindow::updateGitBranch(const QString& branch) {
    if (branch.isEmpty()) {
        branch_label_->setText("");
    } else {
        branch_label_->setText(" " + branch);
    }
}

void MainWindow::setupActivityBar() {
    activity_bar_ = new QToolBar("Activity Bar", this);
    activity_bar_->setMovable(false);
    activity_bar_->setOrientation(Qt::Vertical);
    activity_bar_->setIconSize(QSize(24, 24));
    activity_bar_->setStyleSheet(
        "QToolBar { background-color: #333333; border-right: 1px solid #252526; spacing: 10px; padding-top: 10px; }"
        "QToolButton { background: transparent; border: none; padding: 10px; color: #858585; }"
        "QToolButton:hover { color: #ffffff; }"
        "QToolButton:checked { color: #ffffff; border-left: 2px solid #ffffff; }"
    );

    auto* explorer_action = activity_bar_->addAction(style()->standardIcon(QStyle::SP_DirIcon), "Explorer");
    explorer_action->setCheckable(true);
    explorer_action->setChecked(true);
    connect(explorer_action, &QAction::triggered, [this]() {
        sidebar_stack_->setCurrentIndex(0);
        sidebar_stack_->setVisible(true);
    });

    auto* search_action = activity_bar_->addAction(style()->standardIcon(QStyle::SP_FileDialogContentsView), "Search");
    search_action->setCheckable(true);
    connect(search_action, &QAction::triggered, [this]() {
        sidebar_stack_->setCurrentIndex(1);
        sidebar_stack_->setVisible(true);
    });

    auto* group = new QActionGroup(this);
    group->addAction(explorer_action);
    group->addAction(search_action);
    group->setExclusive(true);

    activity_bar_->addSeparator();

    activity_bar_->addAction(style()->standardIcon(QStyle::SP_ComputerIcon), "Settings");
}

void MainWindow::setupSidebar() {
    sidebar_stack_ = new QStackedWidget(this);
    sidebar_stack_->setFixedWidth(250);
    sidebar_stack_->setStyleSheet("QStackedWidget { background-color: #252526; border-right: 1px solid #3c3c3c; }");

    file_explorer_ = new FileExplorer(this);
    file_explorer_->setRootPath(QDir::currentPath());
    connect(file_explorer_, &FileExplorer::fileActivated, this, &MainWindow::onFileOpen);
    
    auto* search_placeholder = new QWidget();
    auto* search_layout = new QVBoxLayout(search_placeholder);
    auto* search_label = new QLabel("Search (Coming Soon)", search_placeholder);
    search_label->setStyleSheet("color: #858585;");
    search_layout->addWidget(search_label);
    search_layout->addStretch();

    sidebar_stack_->addWidget(file_explorer_);
    sidebar_stack_->addWidget(search_placeholder);
}

void MainWindow::onEditFind() {
    find_replace_widget_->showFind();
}

void MainWindow::onEditReplace() {
    find_replace_widget_->showReplace();
}

void MainWindow::onFindNext() {
    auto* editor = qobject_cast<CodeEditor*>(editor_tabs_->currentWidget());
    if (!editor) return;

    QString pattern = find_replace_widget_->findText();
    if (pattern.isEmpty()) return;

    auto result = xenon::features::SearchEngine::findNext(
        editor->toPlainText().toStdString(),
        pattern.toStdString(),
        static_cast<size_t>(editor->textCursor().position()),
        find_replace_widget_->isCaseSensitive(),
        find_replace_widget_->isRegex()
    );

    if (result.offset != std::string::npos) {
        QTextCursor cursor = editor->textCursor();
        cursor.setPosition(static_cast<int>(result.offset));
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, static_cast<int>(result.length));
        editor->setTextCursor(cursor);
    } else {
        // Wrap around
        result = xenon::features::SearchEngine::findNext(
            editor->toPlainText().toStdString(),
            pattern.toStdString(),
            0,
            find_replace_widget_->isCaseSensitive(),
            find_replace_widget_->isRegex()
        );
        if (result.offset != std::string::npos) {
            QTextCursor cursor = editor->textCursor();
            cursor.setPosition(static_cast<int>(result.offset));
            cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, static_cast<int>(result.length));
            editor->setTextCursor(cursor);
        }
    }
}

void MainWindow::onFindPrevious() {
    auto* editor = qobject_cast<CodeEditor*>(editor_tabs_->currentWidget());
    if (!editor) return;

    QString pattern = find_replace_widget_->findText();
    if (pattern.isEmpty()) return;

    auto result = xenon::features::SearchEngine::findPrevious(
        editor->toPlainText().toStdString(),
        pattern.toStdString(),
        static_cast<size_t>(editor->textCursor().selectionStart()),
        find_replace_widget_->isCaseSensitive(),
        find_replace_widget_->isRegex()
    );

    if (result.offset != std::string::npos) {
        QTextCursor cursor = editor->textCursor();
        cursor.setPosition(static_cast<int>(result.offset));
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, static_cast<int>(result.length));
        editor->setTextCursor(cursor);
    }
}

void MainWindow::onReplace() {
    auto* editor = qobject_cast<CodeEditor*>(editor_tabs_->currentWidget());
    if (!editor) return;

    if (editor->textCursor().hasSelection()) {
        editor->textCursor().insertText(find_replace_widget_->replaceText());
        onFindNext();
    } else {
        onFindNext();
    }
}

void MainWindow::onReplaceAll() {
    auto* editor = qobject_cast<CodeEditor*>(editor_tabs_->currentWidget());
    if (!editor) return;

    QString pattern = find_replace_widget_->findText();
    if (pattern.isEmpty()) return;

    auto results = xenon::features::SearchEngine::findAll(
        editor->toPlainText().toStdString(),
        pattern.toStdString(),
        find_replace_widget_->isCaseSensitive(),
        find_replace_widget_->isRegex()
    );

    if (results.empty()) return;

    QTextCursor cursor = editor->textCursor();
    cursor.beginEditBlock();
    
    // Replace from end to start to maintain offsets
    for (auto it = results.rbegin(); it != results.rend(); ++it) {
        cursor.setPosition(static_cast<int>(it->offset));
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, static_cast<int>(it->length));
        cursor.insertText(find_replace_widget_->replaceText());
    }
    
    cursor.endEditBlock();
}

void MainWindow::onFileNew() {
    createNewEditor("Untitled", "");
}

void MainWindow::onFileOpenDialog() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open File", QDir::currentPath());
    if (!fileName.isEmpty()) {
        onFileOpen(fileName);
    }
}

void MainWindow::onFileOpenFolderDialog() {
    QString dirName = QFileDialog::getExistingDirectory(this, "Open Project", QDir::currentPath());
    if (!dirName.isEmpty()) {
        file_explorer_->setRootPath(dirName);
        git_manager_->setWorkingDirectory(dirName);
        
        // Restart LSP for new root
        if (lsp_client_->isRunning()) {
            lsp_client_->stop();
        }
        lsp_client_->start({"clangd"}, dirName);
        
        QDir::setCurrent(dirName);
        statusBar()->showMessage("Opened Project: " + dirName, 3000);
    }
}

void MainWindow::onFileSave() {
    auto* editor = qobject_cast<CodeEditor*>(editor_tabs_->currentWidget());
    if (!editor) return;

    QString path = editor_tabs_->tabToolTip(editor_tabs_->currentIndex());
    if (path == "Untitled") {
        onFileSaveAs();
        return;
    }

    QFile file(path);
    if (file.open(QFile::WriteOnly | QFile::Text)) {
        QTextStream out(&file);
        out << editor->toPlainText();
        editor->document()->setModified(false);
        if (lsp_client_->isInitialized()) {
            lsp_client_->didSave(QUrl::fromLocalFile(path).toString());
        }
        statusBar()->showMessage("Saved: " + path, 3000);
    }
}

void MainWindow::onFileSaveAs() {
    auto* editor = qobject_cast<CodeEditor*>(editor_tabs_->currentWidget());
    if (!editor) return;

    QString fileName = QFileDialog::getSaveFileName(this, "Save As", QDir::currentPath());
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QFile::WriteOnly | QFile::Text)) {
            QTextStream out(&file);
            out << editor->toPlainText();
            
            QFileInfo fi(fileName);
            int index = editor_tabs_->currentIndex();
            editor_tabs_->setTabText(index, fi.fileName());
            editor_tabs_->setTabToolTip(index, fileName);
            editor->document()->setModified(false);
            if (lsp_client_->isInitialized()) {
                lsp_client_->didSave(QUrl::fromLocalFile(fileName).toString());
            }
            statusBar()->showMessage("Saved: " + fileName, 3000);
        }
    }
}

void MainWindow::onEditUndo() {
    auto* editor = qobject_cast<CodeEditor*>(editor_tabs_->currentWidget());
    if (editor) {
        editor->undo();
    }
}

void MainWindow::onEditRedo() {
    auto* editor = qobject_cast<CodeEditor*>(editor_tabs_->currentWidget());
    if (editor) {
        editor->redo();
    }
}

void MainWindow::onFileOpen(const QString& path) {
    // Check if already open
    for (int i = 0; i < editor_tabs_->count(); ++i) {
        if (editor_tabs_->tabToolTip(i) == path) {
            editor_tabs_->setCurrentIndex(i);
            return;
        }
    }

    QFile file(path);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        createNewEditor(path, QString::fromUtf8(file.readAll()));
    }
}

void MainWindow::createNewEditor(const QString& path, const QString& content) {
    auto* editor = new CodeEditor(this);
    editor->setPlainText(content);
    editor->document()->setModified(false);
    
    QFileInfo fi(path);
    int index = editor_tabs_->addTab(editor, fi.fileName());
    editor_tabs_->setTabToolTip(index, path);
    editor_tabs_->setCurrentIndex(index);

    // LSP: notify file open
    if (lsp_client_->isInitialized()) {
        lsp_client_->didOpen(QUrl::fromLocalFile(path).toString(), "cpp", content);
    }

    static std::unordered_map<CodeEditor*, int> versions;
    versions[editor] = 1;

    connect(editor->document(), &QTextDocument::contentsChanged, [this, editor, path]() {
        if (lsp_client_->isInitialized()) {
            lsp_client_->didChange(QUrl::fromLocalFile(path).toString(), editor->toPlainText(), ++versions[editor]);
        }
    });

    connect(editor->document(), &QTextDocument::modificationChanged, [this, editor](bool changed) {
        int idx = editor_tabs_->indexOf(editor);
        if (idx == -1) return;

        QString title = QFileInfo(editor_tabs_->tabToolTip(idx)).fileName();
        if (changed) {
            editor_tabs_->setTabText(idx, title + " ●");
        } else {
            editor_tabs_->setTabText(idx, title);
        }
    });
}

void MainWindow::onEditorTabClosed(int index) {
    auto* editor = qobject_cast<CodeEditor*>(editor_tabs_->widget(index));
    if (editor && editor->document()->isModified()) {
        auto result = QMessageBox::warning(this, "Unsaved Changes",
            QString("The document '%1' has unsaved changes. Do you want to save them?").arg(editor_tabs_->tabText(index).remove(" ●")),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (result == QMessageBox::Save) {
            editor_tabs_->setCurrentIndex(index);
            onFileSave();
        } else if (result == QMessageBox::Cancel) {
            return;
        }
    }

    auto* widget = editor_tabs_->widget(index);
    if (qobject_cast<CodeEditor*>(widget)) {
        if (lsp_client_->isInitialized()) {
            lsp_client_->didClose(QUrl::fromLocalFile(editor_tabs_->tabToolTip(index)).toString());
        }
    }
    editor_tabs_->removeTab(index);
    delete widget;
}

void MainWindow::closeEvent(QCloseEvent* event) {
    while (editor_tabs_->count() > 0) {
        int initial_count = editor_tabs_->count();
        onEditorTabClosed(0);
        if (editor_tabs_->count() == initial_count) {
            event->ignore();
            return;
        }
    }
    event->accept();
}

void MainWindow::onCommandPalette() {
    command_palette_->showPalette();
}

void MainWindow::onQuickOpen() {
    quick_open_dialog_->showDialog(QDir::currentPath());
}

void MainWindow::onCompletionRequested() {
    auto* editor = qobject_cast<CodeEditor*>(editor_tabs_->currentWidget());
    if (!editor || !lsp_client_->isInitialized()) return;

    QString path = editor_tabs_->tabToolTip(editor_tabs_->currentIndex());
    QTextCursor cursor = editor->textCursor();
    
    lsp_client_->completion(
        QUrl::fromLocalFile(path).toString(),
        cursor.blockNumber(),
        cursor.columnNumber()
    );
}

void MainWindow::onCompletionReceived(int id, const QList<xenon::lsp::CompletionItem>& items) {
    Q_UNUSED(id);
    auto* editor = qobject_cast<CodeEditor*>(editor_tabs_->currentWidget());
    if (!editor) return;

    // Show completion widget at cursor position
    // Use viewport to map coordinates correctly
    QPoint pos = editor->viewport()->mapToGlobal(editor->cursorRect().bottomRight());
    completion_widget_->showCompletions(pos, items);
}

void MainWindow::onCompletionSelected(const QString& text) {
    auto* editor = qobject_cast<CodeEditor*>(editor_tabs_->currentWidget());
    if (!editor) return;

    QTextCursor cursor = editor->textCursor();
    cursor.beginEditBlock();
    
    // Replace the current word part already typed
    cursor.select(QTextCursor::WordUnderCursor);
    QString word = cursor.selectedText();
    
    // If the completion text starts with the word already typed, replace it
    if (text.startsWith(word, Qt::CaseInsensitive)) {
        cursor.insertText(text);
    } else {
        // Just insert if it doesn't match prefix logic (simple fallback)
        cursor.movePosition(QTextCursor::EndOfWord);
        cursor.insertText(text);
    }
    
    cursor.endEditBlock();
    editor->setTextCursor(cursor);
    editor->setFocus();
}

void MainWindow::onDefinitionReceived(int id, const QString& uri, int line, int col) {
    Q_UNUSED(id);
    QString path = QUrl(uri).toLocalFile();
    onFileOpen(path);
    
    auto* editor = qobject_cast<CodeEditor*>(editor_tabs_->currentWidget());
    if (editor) {
        QTextCursor cursor = editor->textCursor();
        cursor.movePosition(QTextCursor::Start);
        for (int i = 0; i < line; ++i) cursor.movePosition(QTextCursor::Down);
        for (int i = 0; i < col; ++i) cursor.movePosition(QTextCursor::Right);
        editor->setTextCursor(cursor);
        editor->ensureCursorVisible();
    }
}

void MainWindow::setupMenus() {
    auto* file_menu = menuBar()->addMenu("&File");
    file_menu->addAction("&New File", QKeySequence::New, this, &MainWindow::onFileNew);
    file_menu->addAction("&Open File...", QKeySequence::Open, this, &MainWindow::onFileOpenDialog);
    file_menu->addAction("Open Folder...", QKeySequence("Ctrl+Alt+O"), this, &MainWindow::onFileOpenFolderDialog);
    file_menu->addAction("Quick Open", QKeySequence("Ctrl+P"), this, &MainWindow::onQuickOpen);
    file_menu->addSeparator();
    file_menu->addAction("&Save", QKeySequence::Save, this, &MainWindow::onFileSave);
    file_menu->addAction("Save &As...", QKeySequence::SaveAs, this, &MainWindow::onFileSaveAs);
    file_menu->addSeparator();
    file_menu->addAction("&Quit", QKeySequence::Quit, qApp, &QApplication::quit);

    auto* edit_menu = menuBar()->addMenu("&Edit");
    edit_menu->addAction("&Undo", QKeySequence::Undo, this, &MainWindow::onEditUndo);
    edit_menu->addAction("&Redo", QKeySequence::Redo, this, &MainWindow::onEditRedo);
    edit_menu->addSeparator();
    edit_menu->addAction("&Find", QKeySequence::Find, this, &MainWindow::onEditFind);
    edit_menu->addAction("&Replace", QKeySequence::Replace, this, &MainWindow::onEditReplace);
    edit_menu->addSeparator();
    edit_menu->addAction("Go to Definition", QKeySequence(Qt::Key_F12), [this]() {
        auto* editor = qobject_cast<CodeEditor*>(editor_tabs_->currentWidget());
        if (editor && lsp_client_->isInitialized()) {
            QString path = editor_tabs_->tabToolTip(editor_tabs_->currentIndex());
            QTextCursor cursor = editor->textCursor();
            lsp_client_->definition(QUrl::fromLocalFile(path).toString(), cursor.blockNumber(), cursor.columnNumber());
        }
    });
#ifdef Q_OS_MAC
    edit_menu->addAction("Show Completions", QKeySequence(Qt::MetaModifier | Qt::Key_Space), this, &MainWindow::onCompletionRequested);
#else
    edit_menu->addAction("Show Completions", QKeySequence("Ctrl+Space"), this, &MainWindow::onCompletionRequested);
#endif

    auto* view_menu = menuBar()->addMenu("&View");
    view_menu->addAction("Command Palette", QKeySequence("Ctrl+Shift+P"), this, &MainWindow::onCommandPalette);
    view_menu->addSeparator();
    view_menu->addAction("Toggle Sidebar", QKeySequence("Ctrl+B"), [this]() {
        sidebar_stack_->setVisible(!sidebar_stack_->isVisible());
    });
    view_menu->addAction("Toggle Terminal", QKeySequence("Ctrl+`"), [this]() {
        terminal_widget_->setVisible(!terminal_widget_->isVisible());
    });
}

} // namespace xenon::ui
