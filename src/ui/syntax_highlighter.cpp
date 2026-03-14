#include "ui/syntax_highlighter.hpp"

namespace xenon::ui {

SyntaxHighlighter::SyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent) {
    HighlightingRule rule;

    keywordFormat.setForeground(QColor("#569cd6"));
    keywordFormat.setFontWeight(QFont::Bold);
    const QStringList keywordPatterns = {
        QStringLiteral("\\bchar\\b"), QStringLiteral("\\bclass\\b"), QStringLiteral("\\bconst\\b"),
        QStringLiteral("\\bdouble\\b"), QStringLiteral("\\benum\\b"), QStringLiteral("\\bexplicit\\b"),
        QStringLiteral("\\bfriend\\b"), QStringLiteral("\\binline\\b"), QStringLiteral("\\bint\\b"),
        QStringLiteral("\\blong\\b"), QStringLiteral("\\bnamespace\\b"), QStringLiteral("\\boperator\\b"),
        QStringLiteral("\\bprivate\\b"), QStringLiteral("\\bprotected\\b"), QStringLiteral("\\bpublic\\b"),
        QStringLiteral("\\bshort\\b"), QStringLiteral("\\bsignals\\b"), QStringLiteral("\\bsigned\\b"),
        QStringLiteral("\\bslots\\b"), QStringLiteral("\\bstatic\\b"), QStringLiteral("\\bstruct\\b"),
        QStringLiteral("\\btemplate\\b"), QStringLiteral("\\btypedef\\b"), QStringLiteral("\\btypename\\b"),
        QStringLiteral("\\bunion\\b"), QStringLiteral("\\bunsigned\\b"), QStringLiteral("\\bvirtual\\b"),
        QStringLiteral("\\bvoid\\b"), QStringLiteral("\\bvolatile\\b"), QStringLiteral("\\bbool\\b"),
        QStringLiteral("\\bif\\b"), QStringLiteral("\\belse\\b"), QStringLiteral("\\bfor\\b"),
        QStringLiteral("\\bwhile\\b"), QStringLiteral("\\breturn\\b"), QStringLiteral("\\bswitch\\b"),
        QStringLiteral("\\bcase\\b")
    };
    for (const QString& pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    classFormat.setForeground(QColor("#4ec9b0"));
    rule.pattern = QRegularExpression(QStringLiteral("\\b[A-Z][A-Za-z0-9_]*\\b"));
    rule.format = classFormat;
    highlightingRules.append(rule);

    singleLineCommentFormat.setForeground(QColor("#6a9955"));
    rule.pattern = QRegularExpression(QStringLiteral("//[^\n]*"));
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    multiLineCommentFormat.setForeground(QColor("#6a9955"));

    quotationFormat.setForeground(QColor("#ce9178"));
    rule.pattern = QRegularExpression(QStringLiteral("\".*\""));
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    functionFormat.setForeground(QColor("#dcdcaa"));
    rule.pattern = QRegularExpression(QStringLiteral("\\b[A-Za-z0-9_]+(?=\\()"));
    rule.format = functionFormat;
    highlightingRules.append(rule);

    commentStartExpression = QRegularExpression(QStringLiteral("/\\*"));
    commentEndExpression = QRegularExpression(QStringLiteral("\\*/"));
}

void SyntaxHighlighter::highlightBlock(const QString& text) {
    for (const HighlightingRule& rule : std::as_const(highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(static_cast<int>(match.capturedStart()), static_cast<int>(match.capturedLength()), rule.format);
        }
    }
    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = static_cast<int>(text.indexOf(commentStartExpression));

    while (startIndex >= 0) {
        QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
        int endIndex = static_cast<int>(match.capturedStart());
        int commentLength = 0;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = static_cast<int>(text.length()) - startIndex;
        } else {
            commentLength = endIndex - startIndex + static_cast<int>(match.capturedLength());
        }
        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = static_cast<int>(text.indexOf(commentStartExpression, startIndex + commentLength));
    }
}

} // namespace xenon::ui
