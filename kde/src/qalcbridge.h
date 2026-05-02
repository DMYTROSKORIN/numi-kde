#ifndef QALCBRIDGE_H
#define QALCBRIDGE_H

#include <QObject>
#include <QString>
#include <QList>
#include <QTimer>
#include <QJsonObject>
#include <QHash>

class QNetworkAccessManager;
class QNetworkReply;

class Calculator;

struct LineResult {
    bool ok = false;
    QString result;
    QString error;
    QString highlightedHtml;
    bool hasNumericValue = false;
    double numericValue = 0.0;
    QString totalKey;
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
    QString highlightLine(const QString &line) const;

    void fetchCryptoRates();
    void applyCryptoRates(const QJsonObject &rates);

signals:
    void cryptoRatesUpdated();

private slots:
    void onCryptoReply(QNetworkReply *reply);

private:
    QString convertCryptoExpression(const QString &expression, bool *converted) const;

    Calculator *m_calc;
    SyntaxHighlighter *m_highlighter;
    int m_decimalPlaces = 3;
    QNetworkAccessManager *m_nam;
    QTimer *m_cryptoRefreshTimer;
    QHash<QString, double> m_cryptoUsdRates;
};

#endif // QALCBRIDGE_H
