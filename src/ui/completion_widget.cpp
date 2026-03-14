#include "ui/completion_widget.hpp"
#include <QKeyEvent>

namespace xenon::ui {

CompletionWidget::CompletionWidget(QWidget* parent)
    : QListWidget(parent) {
    
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setFixedWidth(300);
    setMaximumHeight(200);
    setStyleSheet(
        "QListWidget { background-color: #252526; color: #cccccc; border: 1px solid #454545; outline: none; }"
        "QListWidget::item { padding: 4px; border-bottom: 1px solid #2d2d2d; }"
        "QListWidget::item:selected { background-color: #094771; color: #ffffff; }"
    );

    connect(this, &QListWidget::itemDoubleClicked, this, &CompletionWidget::onItemSelected);
}

void CompletionWidget::showCompletions(const QPoint& pos, const QList<xenon::lsp::CompletionItem>& items) {
    clear();
    for (const auto& item : items) {
        auto* listItem = new QListWidgetItem(item.label, this);
        listItem->setData(Qt::UserRole, item.insertText.isEmpty() ? item.label : item.insertText);
        listItem->setToolTip(item.detail);
    }
    
    if (count() > 0) {
        setCurrentRow(0);
        move(pos);
        show();
        setFocus();
    }
}

void CompletionWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        onItemSelected(currentItem());
        event->accept();
    } else if (event->key() == Qt::Key_Escape) {
        hide();
        event->accept();
    } else {
        QListWidget::keyPressEvent(event);
    }
}

void CompletionWidget::onItemSelected(QListWidgetItem* item) {
    if (item) {
        emit completionSelected(item->data(Qt::UserRole).toString());
        hide();
    }
}

} // namespace xenon::ui
