#ifndef SYNTAXHIGHLIGHTER_H
#define SYNTAXHIGHLIGHTER_H

#include <QString>
#include <QSet>
#include <QStringList>
#include <QRegularExpression>
#include <QHash>

class Calculator;

class SyntaxHighlighter {
public:
    explicit SyntaxHighlighter(Calculator *calc);

    // singleWordVars: user-defined variable names without spaces (matched by tokenizer)
    // multiWordVarsSorted: user-defined variable names with spaces, sorted by length descending
    QString highlightLine(const QString &line,
                          const QSet<QString> &singleWordVars,
                          const QStringList &multiWordVarsSorted = {});

private:
    QString tokenizeSegment(const QString &text, const QSet<QString> &variables);
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
