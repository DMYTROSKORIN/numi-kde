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
};

class QalcBridge : public QObject {
    Q_OBJECT
public:
    explicit QalcBridge(QObject *parent = nullptr);
    ~QalcBridge();

    QList<LineResult> evaluateDocument(const QString &source);

private:
    Calculator *m_calc;
};

#endif // QALCBRIDGE_H
