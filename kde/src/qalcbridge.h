#ifndef QALCBRIDGE_H
#define QALCBRIDGE_H

#include <QObject>
#include <QString>
#include <QList>

class Calculator;

struct LineResult {
    bool ok;
    QString result;
    QString error;
    QString highlightedHtml;
};

class SyntaxHighlighter;

class QalcBridge : public QObject {
    Q_OBJECT
public:
    explicit QalcBridge(QObject *parent = nullptr);
    ~QalcBridge();

    QList<LineResult> evaluateDocument(const QString &source);
    void setDecimalPlaces(int places);
    QString getCompletion(const QString &prefix);

private:
    Calculator *m_calc;
    SyntaxHighlighter *m_highlighter;
    int m_decimalPlaces = 3;
};

#endif // QALCBRIDGE_H
