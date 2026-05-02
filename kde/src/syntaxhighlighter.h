#ifndef SYNTAXHIGHLIGHTER_H
#define SYNTAXHIGHLIGHTER_H

#include <QString>
#include <QSet>
#include <QRegularExpression>

class Calculator;

class SyntaxHighlighter {
public:
    explicit SyntaxHighlighter(Calculator *calc);

    QString highlightLine(const QString &line, const QSet<QString> &variables);

private:
    QString escapeHtml(const QString &input);
    QString escapeWhitespace(const QString &input);
    QString classify(const QString &token, const QSet<QString> &variables);

    Calculator *m_calc;
    QRegularExpression m_tokenRegex;
    QSet<QString> m_operatorWords;
    QSet<QString> m_operatorSymbols;
    QHash<QString, QString> m_classificationCache;
};

#endif // SYNTAXHIGHLIGHTER_H
