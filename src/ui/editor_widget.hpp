#pragma once

#include <QPlainTextEdit>
#include <QWidget>
#include "ui/syntax_highlighter.hpp"

namespace xenon::ui {

class CodeEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit CodeEditor(QWidget* parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent* event);
    int lineNumberAreaWidth();

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect& rect, int dy);

private:
    QWidget* line_number_area_;
    SyntaxHighlighter* highlighter_;
};

class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(CodeEditor* editor) : QWidget(editor), editor_(editor) {}

    QSize sizeHint() const override {
        return QSize(editor_->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        editor_->lineNumberAreaPaintEvent(event);
    }

private:
    CodeEditor* editor_;
};

} // namespace xenon::ui
