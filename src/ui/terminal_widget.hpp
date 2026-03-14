#pragma once

#include <QPlainTextEdit>
#include <QProcess>

namespace xenon::ui {

class TerminalWidget : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit TerminalWidget(QWidget* parent = nullptr);
    ~TerminalWidget() override;

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QProcess process_;
};

} // namespace xenon::ui
