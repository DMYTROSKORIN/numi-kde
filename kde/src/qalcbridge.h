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
    void setDecimalPlaces(int places);

private:
    Calculator *m_calc;
    int m_decimalPlaces = 3;
};

#endif // QALCBRIDGE_H
