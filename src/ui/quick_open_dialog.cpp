#include "ui/quick_open_dialog.hpp"
#include <QKeyEvent>
#include <QDirIterator>
#include <QFileInfo>

namespace xenon::ui {

QuickOpenDialog::QuickOpenDialog(QWidget* parent)
    : QDialog(parent, Qt::FramelessWindowHint | Qt::Popup) {
    
    setFixedWidth(600);
    setStyleSheet(
        "QDialog { background-color: #252526; border: 1px solid #454545; border-radius: 6px; }"
        "QLineEdit { background-color: #3c3c3c; color: #ffffff; border: none; padding: 10px; font-size: 14pt; border-radius: 4px; margin: 5px; }"
        "QListWidget { background-color: transparent; border: none; color: #cccccc; font-size: 12pt; outline: none; }"
        "QListWidget::item { padding: 10px; border-bottom: 1px solid #2d2d2d; }"
        "QListWidget::item:selected { background-color: #094771; color: #ffffff; }"
    );

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    search_edit_ = new QLineEdit(this);
    search_edit_->setPlaceholderText("Search files by name...");
    layout->addWidget(search_edit_);

    list_widget_ = new QListWidget(this);
    list_widget_->setMinimumHeight(300);
    layout->addWidget(list_widget_);

    connect(search_edit_, &QLineEdit::textChanged, this, &QuickOpenDialog::onTextChanged);
    connect(list_widget_, &QListWidget::itemDoubleClicked, this, &QuickOpenDialog::onItemSelected);
    
    search_edit_->installEventFilter(this);
}

void QuickOpenDialog::showDialog(const QString& rootPath) {
    root_path_ = rootPath;
    refreshFiles(rootPath);
    search_edit_->clear();
    
    // Position in top-center of parent
    QWidget* p = parentWidget();
    if (p) {
        move(p->geometry().center().x() - width() / 2, p->geometry().top() + 50);
    }
    
    show();
    search_edit_->setFocus();
}

bool QuickOpenDialog::eventFilter(QObject* obj, QEvent* event) {
    if (obj == search_edit_ && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Down) {
            int row = (list_widget_->currentRow() + 1) % list_widget_->count();
            while (row < list_widget_->count() && list_widget_->item(row)->isHidden()) {
                row = (row + 1) % list_widget_->count();
            }
            list_widget_->setCurrentRow(row);
            return true;
        } else if (keyEvent->key() == Qt::Key_Up) {
            int row = (list_widget_->currentRow() - 1 + list_widget_->count()) % list_widget_->count();
            while (row >= 0 && list_widget_->item(row)->isHidden()) {
                row = (row - 1 + list_widget_->count()) % list_widget_->count();
            }
            list_widget_->setCurrentRow(row);
            return true;
        } else if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            onItemSelected(list_widget_->currentItem());
            return true;
        } else if (keyEvent->key() == Qt::Key_Escape) {
            hide();
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}

void QuickOpenDialog::onTextChanged(const QString& text) {
    list_widget_->clear();
    if (text.isEmpty()) {
        for (const auto& f : all_files_) {
            list_widget_->addItem(f);
        }
    } else {
        for (const auto& f : all_files_) {
            if (f.contains(text, Qt::CaseInsensitive)) {
                list_widget_->addItem(f);
            }
        }
    }
    list_widget_->setCurrentRow(0);
}

void QuickOpenDialog::onItemSelected(QListWidgetItem* item) {
    if (item) {
        QString relativePath = item->text();
        QString absolutePath = QDir(root_path_).absoluteFilePath(relativePath);
        emit fileSelected(absolutePath);
        hide();
    }
}

void QuickOpenDialog::refreshFiles(const QString& rootPath) {
    all_files_.clear();
    list_widget_->clear();
    
    QDir root(rootPath);
    QDirIterator it(rootPath, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString path = it.next();
        
        // Skip hidden and build directories
        if (path.contains("/.git/") || path.contains("/build/") || path.contains("/node_modules/")) {
            continue;
        }
        
        all_files_.append(root.relativeFilePath(path));
    }
    
    for (const auto& f : all_files_) {
        list_widget_->addItem(f);
    }
    list_widget_->setCurrentRow(0);
}

} // namespace xenon::ui
