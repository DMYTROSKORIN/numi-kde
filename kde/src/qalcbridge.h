#ifndef QALCBRIDGE_H
#define QALCBRIDGE_H

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include <QTimer>
#include <QJsonObject>
#include <QHash>
#include <QMutex>
#include <QSet>
#include <atomic>
#include <functional>
#include "lineresult.h"

class QNetworkAccessManager;
class QNetworkReply;

class Calculator;
class Unit;

class SyntaxHighlighter;

/**
 * Threading contract: everything that reaches libqalculate's Calculator —
 * evaluateDocument(), getCompletion(), highlightLine(), setDecimalPlaces(),
 * setDefaultCurrency(), the rate appliers — must run on ONE thread (the engine
 * process uses a single-thread pool). The library stalls its own calculation
 * thread when driven from two threads, even with our mutex serialising them.
 * Thread-safe from any thread: getCompletions() (snapshot), abortCalculation(),
 * networkStatus().
 */
class QalcBridge : public QObject {
    Q_OBJECT
public:
    explicit QalcBridge(QObject *parent = nullptr);
    ~QalcBridge();

    /// Evaluates the whole document. `progress(index)` is invoked (on the calling
    /// thread) after each line; lines listed in `skipLines` are not evaluated
    /// and report "Calculation took too long" (the GUI's watchdog poisoned them).
    QList<LineResult> evaluateDocument(const QString &source,
                                       const std::function<void(int)> &progress = {},
                                       const QSet<int> &skipLines = {});
    void setDecimalPlaces(int places);
    /// Tells a running evaluateDocument() (other thread) to skip its remaining lines.
    void abortCalculation();
    QString getCompletion(const QString &prefix);
    QStringList getCompletions(const QString &lineContext);
    QString highlightLine(const QString &line) const;

    void fetchFiatRates();
    void fetchCryptoRates();
    void applyFiatRates(const QJsonObject &rates);
    void applyCryptoRates(const QJsonObject &rates);
    void setDefaultCurrency(const QString &currency);

    enum class NetworkStatus { Idle, Fetching, Success, Error, Partial };
    NetworkStatus networkStatus() const;

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
    bool updateUsdAliasUnit(const QString &name, double usdPerUnit, Unit *usd);
    void rebuildNameIndexLocked();

    enum class NameKind { Unit, Currency, Function };
    struct NameEntry { QString name; QString description; NameKind kind; };

    Calculator *m_calc;
    SyntaxHighlighter *m_highlighter;
    int m_decimalPlaces = 3;
    QNetworkAccessManager *m_nam;
    QTimer *m_ratesRefreshTimer;
    QHash<QString, double> m_fiatUsdRates;
    QHash<QString, double> m_cryptoUsdRates;
    // Per-document variable state: populated during evaluateDocument, cleared each run.
    QHash<QString, QString> m_varCurrencyTag;  // displayName → currency symbol (e.g. "USD")
    QHash<QString, double>  m_varNumericValue; // displayName → numeric value in that currency
    // User variable name mapping: display name (may contain spaces) ↔ internal (underscore) name.
    QMap<QString, QString> m_displayToInternal;   // "Слоны в обороте" → "Слоны_в_обороте"
    QMap<QString, QString> m_internalToDisplay;   // reverse
    QMap<QString, QString> m_userVarDisplayValues; // displayName → last formatted value for autocomplete
    // Fallback output currency for mixed-crypto expressions (e.g. "1 BTC + 1 ETH").
    // Defaults to "USD". Configurable via Settings.
    QString m_defaultCurrency = QStringLiteral("USD");
    mutable QMutex m_calcMutex;
    // Set by abortCalculation(); evaluateDocument() skips the remaining lines.
    std::atomic<bool> m_abortRequested{false};
    // Autocomplete snapshot: written at the end of evaluateDocument() (worker thread),
    // read by getCompletions() (GUI thread). Guarded by its own mutex so the GUI never
    // waits on a long calculation.
    mutable QMutex m_snapshotMutex;
    QStringList m_snapshotVarNames;
    QHash<QString, QString> m_snapshotVarValues;
    QList<NameEntry> m_nameIndex;      // units/currencies/functions for autocomplete (snapshot mutex)
    bool m_nameIndexDirty = true;      // set under the calc mutex when units change
    NetworkStatus m_fiatStatus   = NetworkStatus::Idle;
    NetworkStatus m_cryptoStatus = NetworkStatus::Idle;

    void loadRatesFromCache();
    void saveRatesToCache(bool isCrypto, const QJsonObject &rates);
};

#endif // QALCBRIDGE_H
