#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QSplitter>
#include <QTabWidget>
#include <QStatusBar>
#include <QToolBar>
#include <QLabel>
#include <QCloseEvent>
#include <memory>

#include "ui/file_explorer.hpp"
#include "ui/editor_widget.hpp"
#include "ui/terminal_widget.hpp"
#include "ui/command_palette.hpp"
#include "ui/find_replace_widget.hpp"
#include "git/git_manager.hpp"

namespace xenon::ui {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onFileNew();
    void onFileOpenDialog();
    void onFileSave();
    void onFileSaveAs();
    void onEditUndo();
    void onEditRedo();
    void onEditFind();
    void onEditReplace();
    void onFindNext();
    void onFindPrevious();
    void onReplace();
    void onReplaceAll();
    void onFileOpen(const QString& path);
    void onEditorTabClosed(int index);
    void updateGitBranch(const QString& branch);
    void onCommandPalette();

private:
    void setupUI();
    void setupMenus();
    void setupActivityBar();
    void setupSidebar();
    void createNewEditor(const QString& path, const QString& content);

    QToolBar* activity_bar_;
    QStackedWidget* sidebar_stack_;
    QSplitter* main_splitter_;
    QSplitter* content_splitter_;
    QTabWidget* editor_tabs_;
    FileExplorer* file_explorer_;
    TerminalWidget* terminal_widget_;
    CommandPalette* command_palette_;
    FindReplaceWidget* find_replace_widget_;
    
    QLabel* branch_label_;
    std::unique_ptr<xenon::git::GitManager> git_manager_;
};

} // namespace xenon::ui
