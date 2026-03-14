#include "ui/terminal_widget.hpp"
#include <QKeyEvent>

namespace xenon::ui {

TerminalWidget::TerminalWidget(QWidget* parent)
    : QPlainTextEdit(parent) {
    
    setStyleSheet(
        "QPlainTextEdit { background-color: #1e1e1e; color: #d4d4d4; border-top: 1px solid #3c3c3c; font-family: 'Menlo', 'Monaco', 'Courier New'; font-size: 11pt; }"
    );

    connect(&process_, &QProcess::readyReadStandardOutput, this, &TerminalWidget::onReadyRead);
    connect(&process_, &QProcess::readyReadStandardError, this, &TerminalWidget::onReadyRead);
    connect(&process_, &QProcess::finished, this, &TerminalWidget::onProcessFinished);

#ifdef Q_OS_MAC
    process_.start("/bin/zsh", QStringList() << "-l");
#else
    process_.start("/bin/bash", QStringList() << "-l");
#endif
}

TerminalWidget::~TerminalWidget() {
    if (process_.state() == QProcess::Running) {
        process_.terminate();
        process_.waitForFinished(1000);
    }
}

void TerminalWidget::onReadyRead() {
    QByteArray data = process_.readAllStandardOutput();
    data.append(process_.readAllStandardError());
    insertPlainText(QString::fromUtf8(data));
    ensureCursorVisible();
}

void TerminalWidget::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitStatus);
    insertPlainText(QString("\n[Process finished with exit code %1]\n").arg(exitCode));
}

void TerminalWidget::keyPressEvent(QKeyEvent* event) {
    if (process_.state() == QProcess::Running) {
        QString text = event->text();
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            text = "\n";
        }
        process_.write(text.toUtf8());
    }
    // Don't call base class keyPressEvent to prevent local echo (handled by shell)
    // Or call it if you want local editing (not recommended for simple term)
}

} // namespace xenon::ui
