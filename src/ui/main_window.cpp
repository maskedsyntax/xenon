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

#include <QMessageBox>

#include "features/search_engine.hpp"

namespace xenon::ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    
    setWindowTitle("Xenon");
    resize(1200, 800);

    git_manager_ = std::make_unique<xenon::git::GitManager>(this);
    connect(git_manager_.get(), &xenon::git::GitManager::branchChanged, this, &MainWindow::updateGitBranch);

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
        editor->textCursor().position(),
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
        editor->textCursor().selectionStart(),
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

void MainWindow::setupMenus() {
    auto* file_menu = menuBar()->addMenu("&File");
    file_menu->addAction("&New File", QKeySequence::New, this, &MainWindow::onFileNew);
    file_menu->addAction("&Open...", QKeySequence::Open, this, &MainWindow::onFileOpenDialog);
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

    auto* view_menu = menuBar()->addMenu("&View");
    view_menu->addAction("Command Palette", QKeySequence("Ctrl+Shift+P"), this, &MainWindow::onCommandPalette);
}

} // namespace xenon::ui
