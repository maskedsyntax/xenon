#pragma once

#include <QListWidget>
#include <QPoint>
#include "lsp/lsp_client.hpp"

namespace xenon::ui {

class CompletionWidget : public QListWidget {
    Q_OBJECT

public:
    explicit CompletionWidget(QWidget* parent = nullptr);

    void showCompletions(const QPoint& pos, const QList<xenon::lsp::CompletionItem>& items);

signals:
    void completionSelected(const QString& text);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onItemSelected(QListWidgetItem* item);
};

} // namespace xenon::ui
