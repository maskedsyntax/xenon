#include "ui/command_palette.hpp"
#include <QKeyEvent>
#include <QApplication>
#include <QScreen>

namespace xenon::ui {

CommandPalette::CommandPalette(QWidget* parent)
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
    search_edit_->setPlaceholderText("Type a command...");
    layout->addWidget(search_edit_);

    list_widget_ = new QListWidget(this);
    list_widget_->setMinimumHeight(300);
    layout->addWidget(list_widget_);

    // Initial commands
    QStringList commands = {"File: New File", "File: Open File", "File: Save", "View: Toggle Sidebar", "View: Toggle Terminal", "Git: Pull", "Git: Push"};
    list_widget_->addItems(commands);
    list_widget_->setCurrentRow(0);

    connect(search_edit_, &QLineEdit::textChanged, this, &CommandPalette::onTextChanged);
    connect(list_widget_, &QListWidget::itemDoubleClicked, this, &CommandPalette::onItemSelected);
    
    search_edit_->installEventFilter(this);
}

void CommandPalette::showPalette() {
    search_edit_->clear();
    
    // Position in top-center of parent or screen
    QWidget* p = parentWidget();
    if (p) {
        move(p->geometry().center().x() - width() / 2, p->geometry().top() + 50);
    }
    
    show();
    search_edit_->setFocus();
}

bool CommandPalette::eventFilter(QObject* obj, QEvent* event) {
    if (obj == search_edit_ && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Down) {
            list_widget_->setCurrentRow((list_widget_->currentRow() + 1) % list_widget_->count());
            return true;
        } else if (keyEvent->key() == Qt::Key_Up) {
            list_widget_->setCurrentRow((list_widget_->currentRow() - 1 + list_widget_->count()) % list_widget_->count());
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

void CommandPalette::onTextChanged(const QString& text) {
    for (int i = 0; i < list_widget_->count(); ++i) {
        auto* item = list_widget_->item(i);
        item->setHidden(!item->text().contains(text, Qt::CaseInsensitive));
    }
    list_widget_->setCurrentRow(0);
}

void CommandPalette::onItemSelected(QListWidgetItem* item) {
    if (item) {
        // TODO: Execute command
        hide();
    }
}

} // namespace xenon::ui
