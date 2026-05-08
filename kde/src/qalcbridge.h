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
    void setDefaultCurrency(const QString &currency);

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
    bool tryEvaluateCurrencyExpr(const QString &expr, QString *outResult, QString *outCurrency) const;
    double usdRateForSymbol(const QString &symbol, bool *ok) const;
    bool hasUsdRateForSymbol(const QString &symbol) const;

    Calculator *m_calc;
    SyntaxHighlighter *m_highlighter;
    int m_decimalPlaces = 3;
    QNetworkAccessManager *m_nam;
    QTimer *m_ratesRefreshTimer;
    QHash<QString, double> m_fiatUsdRates;
    QHash<QString, double> m_cryptoUsdRates;
    // Per-document variable state: populated during evaluateDocument, cleared each run.
    QHash<QString, QString> m_varCurrencyTag;  // varName → currency symbol (e.g. "USD")
    QHash<QString, double>  m_varNumericValue; // varName → numeric value in that currency
    // Fallback output currency for mixed-crypto expressions (e.g. "1 BTC + 1 ETH").
    // Defaults to "USD". Configurable via Settings.
    QString m_defaultCurrency = QStringLiteral("USD");
    mutable QMutex m_calcMutex;
    NetworkStatus m_networkStatus = NetworkStatus::Idle;
};

#endif // QALCBRIDGE_H
