#include "ui/file_explorer.hpp"
#include <QHeaderView>

namespace xenon::ui {

FileExplorer::FileExplorer(QWidget* parent)
    : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    model_ = new QFileSystemModel(this);
    model_->setRootPath("");
    model_->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);

    tree_view_ = new QTreeView(this);
    tree_view_->setModel(model_);
    tree_view_->setHeaderHidden(true);
    tree_view_->setAnimated(true);
    tree_view_->setIndentation(20);
    tree_view_->setSortingEnabled(true);
    tree_view_->sortByColumn(0, Qt::AscendingOrder);
    
    // Hide columns except Name
    for (int i = 1; i < model_->columnCount(); ++i) {
        tree_view_->hideColumn(i);
    }

    tree_view_->setStyleSheet(
        "QTreeView { background-color: #252526; border: none; color: #cccccc; }"
        "QTreeView::item:hover { background-color: #2a2d2e; }"
        "QTreeView::item:selected { background-color: #094771; color: #ffffff; }"
    );

    layout->addWidget(tree_view_);

    connect(tree_view_, &QTreeView::activated, [this](const QModelIndex& index) {
        if (!model_->isDir(index)) {
            emit fileActivated(model_->filePath(index));
        }
    });
}

void FileExplorer::setRootPath(const QString& path) {
    model_->setRootPath(path);
    tree_view_->setRootIndex(model_->index(path));
}

} // namespace xenon::ui
