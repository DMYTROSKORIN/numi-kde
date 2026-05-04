#ifndef QALCBRIDGE_H
#define QALCBRIDGE_H

#include <QObject>
#include <QString>
#include <QList>
#include <QTimer>
#include <QJsonObject>
#include <QHash>
#include <QMutex>

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

    void fetchFiatRates();
    void fetchCryptoRates();
    void applyFiatRates(const QJsonObject &rates);
    void applyCryptoRates(const QJsonObject &rates);

    enum class NetworkStatus { Idle, Fetching, Success, Error };
    NetworkStatus networkStatus() const { return m_networkStatus; }

signals:
    void ratesUpdated();
    void networkStatusChanged();

private slots:
    void onFiatReply(QNetworkReply *reply);
    void onCryptoReply(QNetworkReply *reply);

private:
    QString convertCurrencyExpression(const QString &expression, bool *converted) const;
    double usdRateForSymbol(const QString &symbol, bool *ok) const;
    bool hasUsdRateForSymbol(const QString &symbol) const;

    Calculator *m_calc;
    SyntaxHighlighter *m_highlighter;
    int m_decimalPlaces = 3;
    QNetworkAccessManager *m_nam;
    QTimer *m_ratesRefreshTimer;
    QHash<QString, double> m_fiatUsdRates;
    QHash<QString, double> m_cryptoUsdRates;
    mutable QMutex m_calcMutex;
    NetworkStatus m_networkStatus = NetworkStatus::Idle;
};

#endif // QALCBRIDGE_H
