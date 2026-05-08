/**
 * Portions Copyright (c) Nikolas
 * Original MIT License: https://github.com/nikolaeu/numi/blob/master/license.txt
 * Modified by DMYTRO SKORIN under Apache License 2.0
 */

#include "qalcbridge.h"
#include "syntaxhighlighter.h"
#include <libqalculate/qalculate.h>

#ifndef NUMI_KDE_VERSION
#define NUMI_KDE_VERSION "0.0.0"
#endif
#include <QStringList>
#include <QRegularExpression>
#include <QSet>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDateTime>
#include <QTime>
#include <QLocale>
#include <algorithm>
#include <cmath>

// Top-50 crypto CoinGecko IDs → ticker symbols
static const QList<QPair<QString,QString>> CRYPTO_LIST = {
    {"bitcoin","BTC"}, {"ethereum","ETH"}, {"tether","USDT"}, {"binancecoin","BNB"},
    {"solana","SOL"}, {"ripple","XRP"}, {"usd-coin","USDC"}, {"staked-ether","STETH"},
    {"dogecoin","DOGE"}, {"the-open-network","TON"}, {"cardano","ADA"},
    {"avalanche-2","AVAX"}, {"tron","TRX"}, {"shiba-inu","SHIB"},
    {"polkadot","DOT"}, {"bitcoin-cash","BCH"}, {"chainlink","LINK"},
    {"near","NEAR"}, {"litecoin","LTC"}, {"uniswap","UNI"},
    {"aptos","APT"}, {"internet-computer","ICP"}, {"cosmos","ATOM"},
    {"cronos","CRO"}, {"ethereum-classic","ETC"}, {"stellar","XLM"},
    {"filecoin","FIL"}, {"monero","XMR"}, {"okb","OKB"},
    {"hedera-hashgraph","HBAR"}, {"immutable-x","IMX"},
    {"vechain","VET"}, {"arbitrum","ARB"}, {"algorand","ALGO"},
    {"the-sandbox","SAND"}, {"decentraland","MANA"}, {"aave","AAVE"},
    {"quant-network","QNT"}, {"theta-token","THETA"}, {"elrond-erd-2","EGLD"},
    {"axie-infinity","AXS"}, {"eos","EOS"}, {"flow","FLOW"},
    {"tezos","XTZ"}, {"bitcoin-sv","BSV"}, {"neo","NEO"},
    {"kucoin-shares","KCS"}, {"iota","IOTA"}, {"pancakeswap-token","CAKE"},
    {"gala","GALA"}
};

QalcBridge::QalcBridge(QObject *parent) : QObject(parent) {
    m_calc = new Calculator();
    m_calc->loadGlobalDefinitions();
    m_calc->loadExchangeRates();
    m_highlighter = new SyntaxHighlighter(m_calc);

    m_nam = new QNetworkAccessManager(this);

    // Fetch on startup, then refresh every hour.
    m_ratesRefreshTimer = new QTimer(this);
    m_ratesRefreshTimer->setInterval(60 * 60 * 1000);
    connect(m_ratesRefreshTimer, &QTimer::timeout, this, [this]() {
        fetchFiatRates();
        fetchCryptoRates();
    });
    m_ratesRefreshTimer->start();

    fetchFiatRates();
    fetchCryptoRates();
}

QalcBridge::~QalcBridge() {
    delete m_highlighter;
    delete m_calc;
}

void QalcBridge::fetchFiatRates() {
    m_networkStatus = NetworkStatus::Fetching;
    emit networkStatusChanged();

    QNetworkRequest req(QUrl(QStringLiteral("https://api.frankfurter.dev/v2/rates?base=USD")));
    req.setTransferTimeout(10000);
    req.setRawHeader("User-Agent", "numi-kde/" NUMI_KDE_VERSION);
    auto *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onFiatReply(reply);
    });
}

void QalcBridge::fetchCryptoRates() {
    m_networkStatus = NetworkStatus::Fetching;
    emit networkStatusChanged();

    QStringList ids;
    for (const auto &p : CRYPTO_LIST)
        ids << p.first;

    const QString url = QStringLiteral("https://api.coingecko.com/api/v3/simple/price?ids=")
                        + ids.join(QLatin1Char(','))
                        + QStringLiteral("&vs_currencies=usd");
    QNetworkRequest req{QUrl(url)};
    req.setTransferTimeout(10000);
    req.setRawHeader("User-Agent", "numi-kde/" NUMI_KDE_VERSION);
    auto *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onCryptoReply(reply);
    });
}

void QalcBridge::onFiatReply(QNetworkReply *reply) {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        m_networkStatus = NetworkStatus::Error;
        emit networkStatusChanged();
        return;
    }

    const QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
    QJsonObject usdRates;
    usdRates.insert(QStringLiteral("USD"), 1.0);

    if (document.isArray()) {
        const QJsonArray rows = document.array();
        for (const QJsonValue &value : rows) {
            const QJsonObject row = value.toObject();
            if (row.value(QStringLiteral("base")).toString().toUpper() != QStringLiteral("USD"))
                continue;

            const QString quote = row.value(QStringLiteral("quote")).toString().toUpper();
            const double usdToCurrency = row.value(QStringLiteral("rate")).toDouble();
            if (!quote.isEmpty() && usdToCurrency > 0.0)
                usdRates.insert(quote, 1.0 / usdToCurrency);
        }
    } else {
        const QJsonObject quoteRates = document.object().value(QStringLiteral("rates")).toObject();
        for (auto it = quoteRates.constBegin(); it != quoteRates.constEnd(); ++it) {
            const double usdToCurrency = it.value().toDouble();
            if (usdToCurrency > 0.0)
                usdRates.insert(it.key().toUpper(), 1.0 / usdToCurrency);
        }
    }

    if (usdRates.size() > 1) {
        applyFiatRates(usdRates);
        m_networkStatus = NetworkStatus::Success;
        emit networkStatusChanged();
        emit ratesUpdated();
    } else {
        m_networkStatus = NetworkStatus::Error;
        emit networkStatusChanged();
    }
}

void QalcBridge::onCryptoReply(QNetworkReply *reply) {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        m_networkStatus = NetworkStatus::Error;
        emit networkStatusChanged();
        return;
    }

    QJsonObject data = QJsonDocument::fromJson(reply->readAll()).object();
    QJsonObject rates;
    for (const auto &p : CRYPTO_LIST) {
        if (data.contains(p.first)) {
            double price = data[p.first].toObject()["usd"].toDouble();
            if (price > 0)
                rates[p.second] = price;
        }
    }
    if (!rates.isEmpty()) {
        applyCryptoRates(rates);
        m_networkStatus = NetworkStatus::Success;
        emit networkStatusChanged();
        emit ratesUpdated();
    } else {
        m_networkStatus = NetworkStatus::Error;
        emit networkStatusChanged();
    }
}

void QalcBridge::applyFiatRates(const QJsonObject &rates) {
    QMutexLocker locker(&m_calcMutex);

    Unit *usd = m_calc->getUnit("USD");
    if (!usd) return;

    for (const QString &symbol : rates.keys()) {
        const double usdPerCurrency = rates[symbol].toDouble();
        if (usdPerCurrency <= 0.0) continue;
        const QString normalized = symbol.toUpper();
        m_fiatUsdRates.insert(normalized, usdPerCurrency);

        if (normalized == QStringLiteral("USD"))
            continue;

        auto *unit = new AliasUnit(
            "Currency",
            normalized.toStdString(),
            normalized.toStdString(),
            normalized.toStdString(),
            normalized.toStdString(),
            usd,
            std::to_string(usdPerCurrency),
            1,
            "",
            false,
            false,
            true
        );
        m_calc->addUnit(unit, true);
    }
}

void QalcBridge::applyCryptoRates(const QJsonObject &rates) {
    QMutexLocker locker(&m_calcMutex);

    Unit *usd = m_calc->getUnit("USD");
    if (!usd) return;

    for (const QString &symbol : rates.keys()) {
        double price = rates[symbol].toDouble();
        if (price <= 0) continue;
        m_cryptoUsdRates.insert(symbol.toUpper(), price);

        // 1 crypto = price USD  →  relation = price (as string)
        auto *unit = new AliasUnit(
            "Cryptocurrency",       // category
            symbol.toStdString(),   // name
            symbol.toStdString(),   // plural
            symbol.toStdString(),   // singular
            symbol.toStdString(),   // title
            usd,                    // reference unit (USD)
            std::to_string(price),  // 1 symbol = price USD
            1,                      // exponent
            "",                     // inverse (empty = linear)
            false,                  // is_local
            false,                  // is_builtin
            true                    // is_active
        );
        m_calc->addUnit(unit, true); // true = replace if exists
    }
}

void QalcBridge::setDecimalPlaces(int places) {
    m_decimalPlaces = places;
}

void QalcBridge::setDefaultCurrency(const QString &currency) {
    const QString upper = currency.trimmed().toUpper();
    m_defaultCurrency = upper.isEmpty() ? QStringLiteral("USD") : upper;
}

// Format a number: no trailing zeros for integers, max m_decimalPlaces otherwise.
// Uses system locale for thousands separators.
static QString smartFormat(double value, int maxDecimals) {
    if (std::isinf(value) || std::isnan(value)) return QString::number(value);

    QLocale locale;
    // For whole numbers, we still want to avoid .000 if they are exactly integers
    if (value == std::floor(value) && std::abs(value) < 1e15) {
        return locale.toString(static_cast<long long>(value));
    }

    QString s = locale.toString(value, 'f', maxDecimals);

    // Remove unnecessary trailing zeros and the decimal point if it becomes empty
    if (s.contains(locale.decimalPoint())) {
        while (s.endsWith(QLatin1Char('0'))) s.chop(1);
        if (s.endsWith(locale.decimalPoint())) s.chop(1);
    }
    return s;
}

static QString pluralize(int value, const QString &one, const QString &many)
{
    return QStringLiteral("%1 %2").arg(value).arg(value == 1 ? one : many);
}

static QString formatEnglishInteger(qint64 value)
{
    return QLocale(QLocale::English, QLocale::UnitedStates).toString(value);
}

static QString joinEnglishParts(const QStringList &parts)
{
    if (parts.size() <= 1)
        return parts.join(QString());
    if (parts.size() == 2)
        return QStringLiteral("%1 and %2").arg(parts.at(0), parts.at(1));

    QStringList head = parts;
    const QString last = head.takeLast();
    return QStringLiteral("%1 and %2").arg(head.join(QStringLiteral(", ")), last);
}

static QString formatDateSpan(QDate from, QDate to)
{
    if (from > to) {
        std::swap(from, to);
    }

    const qint64 totalDays = from.daysTo(to);
    QDate cursor = from;

    int years = 0;
    while (cursor.addYears(1).isValid() && cursor.addYears(1) <= to) {
        cursor = cursor.addYears(1);
        ++years;
    }

    int months = 0;
    while (cursor.addMonths(1).isValid() && cursor.addMonths(1) <= to) {
        cursor = cursor.addMonths(1);
        ++months;
    }

    const int days = cursor.daysTo(to);
    QStringList parts;
    if (years > 0)
        parts << pluralize(years, QStringLiteral("year"), QStringLiteral("years"));
    if (months > 0)
        parts << pluralize(months, QStringLiteral("month"), QStringLiteral("months"));
    if (days > 0 || parts.isEmpty())
        parts << pluralize(days, QStringLiteral("day"), QStringLiteral("days"));

    return QStringLiteral("%1; %2 days.")
        .arg(joinEnglishParts(parts),
             formatEnglishInteger(totalDays));
}

static QString formatUserDate(const QDate &date)
{
    return date.toString(QStringLiteral("dd.MM.yyyy"));
}

static QDate parseUserDate(const QString &value)
{
    static const QStringList formats = {
        QStringLiteral("d.M.yyyy"),
        QStringLiteral("dd.MM.yyyy"),
        QStringLiteral("d/M/yyyy"),
        QStringLiteral("dd/MM/yyyy"),
        QStringLiteral("d-M-yyyy"),
        QStringLiteral("dd-MM-yyyy"),
        QStringLiteral("yyyy-MM-dd"),
    };

    for (const QString &format : formats) {
        const QDate date = QDate::fromString(value.trimmed(), format);
        if (date.isValid())
            return date;
    }
    return {};
}

static bool tryDateDifference(const QString &trimmed, QString *result)
{
    static QRegularExpression dateMinusDate(
        "^\\s*(\\d{1,4}[./-]\\d{1,2}[./-]\\d{1,4})\\s*-\\s*(\\d{1,4}[./-]\\d{1,2}[./-]\\d{1,4})\\s*$",
        QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression todayMinusDate(
        "^\\s*(?:today|now)\\s*-\\s*(\\d{1,4}[./-]\\d{1,2}[./-]\\d{1,4})\\s*$",
        QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression dateMinusToday(
        "^\\s*(\\d{1,4}[./-]\\d{1,2}[./-]\\d{1,4})\\s*-\\s*(?:today|now)\\s*$",
        QRegularExpression::CaseInsensitiveOption);

    auto match = dateMinusDate.match(trimmed);
    if (match.hasMatch()) {
        const QDate from = parseUserDate(match.captured(1));
        const QDate to = parseUserDate(match.captured(2));
        if (!from.isValid() || !to.isValid())
            return false;
        *result = formatDateSpan(from, to);
        return true;
    }

    match = todayMinusDate.match(trimmed);
    if (match.hasMatch()) {
        const QDate date = parseUserDate(match.captured(1));
        if (!date.isValid())
            return false;
        *result = formatDateSpan(date, QDate::currentDate());
        return true;
    }

    match = dateMinusToday.match(trimmed);
    if (match.hasMatch()) {
        const QDate date = parseUserDate(match.captured(1));
        if (!date.isValid())
            return false;
        *result = formatDateSpan(QDate::currentDate(), date);
        return true;
    }

    return false;
}

static bool tryDateArithmetic(const QString &trimmed, QString *result)
{
    static QRegularExpression dateMath(
        "^\\s*(\\d{1,4}[./-]\\d{1,2}[./-]\\d{1,4})\\s*([+-])\\s*(\\d+)\\s*(day|days|week|weeks|month|months|year|years)\\s*$",
        QRegularExpression::CaseInsensitiveOption);

    const auto match = dateMath.match(trimmed);
    if (!match.hasMatch())
        return false;

    QDate date = parseUserDate(match.captured(1));
    if (!date.isValid())
        return false;

    int amount = match.captured(3).toInt();
    if (match.captured(2) == QStringLiteral("-"))
        amount = -amount;

    const QString unit = match.captured(4).toLower();
    if (unit == QStringLiteral("day") || unit == QStringLiteral("days")) {
        date = date.addDays(amount);
    } else if (unit == QStringLiteral("week") || unit == QStringLiteral("weeks")) {
        date = date.addDays(amount * 7);
    } else if (unit == QStringLiteral("month") || unit == QStringLiteral("months")) {
        date = date.addMonths(amount);
    } else if (unit == QStringLiteral("year") || unit == QStringLiteral("years")) {
        date = date.addYears(amount);
    } else {
        return false;
    }

    *result = formatUserDate(date);
    return true;
}

static bool parseDisplayNumber(const QString &text, double *value)
{
    static QRegularExpression firstNumber(
        "([+-]?(?:\\d{1,3}(?:[\\s.,'’]\\d{3})+|\\d+)(?:[.,]\\d+)?|[+-]?[.,]\\d+)");

    const auto match = firstNumber.match(text);
    if (!match.hasMatch())
        return false;

    QString number = match.captured(1);
    QLocale locale;
    bool ok = false;
    double parsed = locale.toDouble(number, &ok);
    if (!ok) {
        number.remove(QLatin1Char(' '));
        number.remove(QLatin1Char('\''));
        number.remove(QStringLiteral("’"));
        const int lastDot = number.lastIndexOf(QLatin1Char('.'));
        const int lastComma = number.lastIndexOf(QLatin1Char(','));
        const int decimalIndex = std::max(lastDot, lastComma);
        QString normalized;
        for (int i = 0; i < number.size(); ++i) {
            const QChar ch = number.at(i);
            if (ch.isDigit() || ((ch == QLatin1Char('+') || ch == QLatin1Char('-')) && normalized.isEmpty())) {
                normalized.append(ch);
            } else if (i == decimalIndex && (ch == QLatin1Char('.') || ch == QLatin1Char(','))) {
                normalized.append(QLatin1Char('.'));
            }
        }
        parsed = normalized.toDouble(&ok);
    }

    if (!ok)
        return false;
    *value = parsed;
    return true;
}

static QString totalKeyForExpression(const QString &trimmed)
{
    static QRegularExpression conversionTarget(
        "\\b(?:to|in|as)\\s+([A-Za-z_π\\p{L}][\\wπ\\p{L}]*)\\s*$",
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption);

    const auto match = conversionTarget.match(trimmed);
    if (match.hasMatch())
        return match.captured(1).toUpper();

    return QStringLiteral("number");
}

static bool isIncompleteExpression(const QString &trimmed)
{
    if (trimmed.isEmpty())
        return false;

    static QRegularExpression incompletePercent(
        "^\\s*[+-]?(?:\\d+(?:[.,]\\d+)?|\\.\\d+)\\s*%\\s+(?:from|of)\\s*$",
        QRegularExpression::CaseInsensitiveOption);
    if (incompletePercent.match(trimmed).hasMatch())
        return true;

    static QRegularExpression trailingOperator(
        "(?:\\b(?:to|in|as|from|of)|:=|[+*/^=(,-])\\s*$",
        QRegularExpression::CaseInsensitiveOption);
    return trailingOperator.match(trimmed).hasMatch();
}

static bool hasExplicitDivisionByZero(const QString &trimmed)
{
    static QRegularExpression divisionByZero(
        "/\\s*[+-]?0+(?:[.,]0+)?(?=\\s*(?:$|[)+\\-*/^,]))");
    return divisionByZero.match(trimmed).hasMatch();
}

static bool hasCalculationError(Calculator *calc)
{
    bool hasError = false;
    for (CalculatorMessage *message = calc->message(); message; message = calc->nextMessage()) {
        if (message->type() == MESSAGE_ERROR)
            hasError = true;
    }
    calc->clearMessages();
    return hasError;
}

bool QalcBridge::hasUsdRateForSymbol(const QString &symbol) const
{
    const QString normalized = symbol.toUpper();
    return normalized == QStringLiteral("USD")
        || m_fiatUsdRates.contains(normalized)
        || m_cryptoUsdRates.contains(normalized);
}

double QalcBridge::usdRateForSymbol(const QString &symbol, bool *ok) const
{
    const QString normalized = symbol.toUpper();
    if (normalized == QStringLiteral("USD")) {
        *ok = true;
        return 1.0;
    }

    const auto fiat = m_fiatUsdRates.constFind(normalized);
    if (fiat != m_fiatUsdRates.constEnd()) {
        *ok = true;
        return fiat.value();
    }

    const auto crypto = m_cryptoUsdRates.constFind(normalized);
    if (crypto != m_cryptoUsdRates.constEnd()) {
        *ok = true;
        return crypto.value();
    }

    if (!m_calc->getUnit(normalized.toStdString())) {
        *ok = false;
        return 0.0;
    }

    EvaluationOptions eo;
    eo.parse_options.angle_unit = ANGLE_UNIT_RADIANS;
    eo.structuring = STRUCTURING_SIMPLIFY;
    eo.parse_options.unknowns_enabled = false;

    PrintOptions po;
    po.number_fraction_format = FRACTION_DECIMAL;
    po.base = 10;
    po.min_decimals = 0;
    po.max_decimals = 12;
    po.use_min_decimals = false;
    po.use_max_decimals = true;
    po.digit_grouping = DIGIT_GROUPING_LOCALE;

    m_calc->clearMessages();
    MathStructure rateResult;
    m_calc->calculate(&rateResult, QStringLiteral("1 %1 to USD").arg(normalized).toStdString(), 5000, eo);
    if (m_calc->aborted() || hasCalculationError(m_calc) || rateResult.isUndefined() || rateResult.isInfinite()) {
        *ok = false;
        return 0.0;
    }
    if (rateResult.isNumber()) {
        const double value = rateResult.number().floatValue();
        *ok = std::isfinite(value) && value > 0.0;
        return *ok ? value : 0.0;
    }

    const QString out = QString::fromStdString(m_calc->print(rateResult, 2000, po));
    double value = 0.0;
    *ok = parseDisplayNumber(out, &value) && value > 0.0;
    return *ok ? value : 0.0;
}

QString QalcBridge::convertCurrencyExpression(const QString &expression, bool *converted) const
{
    if (converted)
        *converted = false;

    static QRegularExpression currencyConversionRegex(
        "^\\s*([+-]?(?:\\d+(?:\\.\\d+)?|\\.\\d+))\\s+([A-Za-z]{2,5})\\s+to\\s+([A-Za-z]{2,5})\\s*$",
        QRegularExpression::CaseInsensitiveOption);

    const auto match = currencyConversionRegex.match(expression);
    if (!match.hasMatch())
        return {};

    const double amount = match.captured(1).toDouble();
    const QString from = match.captured(2).toUpper();
    const QString to = match.captured(3).toUpper();

    bool fromOk = false;
    bool toOk = false;
    const bool hasManagedRate = hasUsdRateForSymbol(from) || hasUsdRateForSymbol(to);
    if (!hasManagedRate)
        return {};

    const double fromRate = usdRateForSymbol(from, &fromOk);
    const double toRate = usdRateForSymbol(to, &toOk);
    if (!fromOk || !toOk || toRate <= 0.0)
        return {};

    if (converted)
        *converted = true;
    return QStringLiteral("%1 %2").arg(to, smartFormat(amount * fromRate / toRate, m_decimalPlaces));
}

// Unified currency evaluator.
//
// Parses `expr` as a sum of signed terms where each term is one of:
//   NUMBER CURRENCY   (e.g. "200 USD", "1 BTC")
//   CURRENCY NUMBER   (e.g. "USD 200")
//   NUMBER            (dimensionless — treated as output currency units)
//   VARIABLE          (resolved via m_varCurrencyTag / m_varNumericValue)
//
// An optional trailing "to CURR" / "in CURR" overrides the output currency.
// Output currency defaults to the first term that carries a currency tag.
//
// Returns false when the expression contains no currency context at all
// (pure math / dates / physics units) so it safely falls through to libqalculate.
bool QalcBridge::tryEvaluateCurrencyExpr(const QString &rawExpr,
                                          QString *outResult,
                                          QString *outCurrency) const
{
    QString expr = rawExpr.trimmed();
    if (expr.isEmpty())
        return false;

    // Strip trailing "to CURR" / "in CURR"
    QString targetCurrency;
    {
        static QRegularExpression toRx(
            "\\b(?:to|in)\\s+([A-Za-z]{2,5})\\s*$",
            QRegularExpression::CaseInsensitiveOption);
        const auto m = toRx.match(expr);
        if (m.hasMatch()) {
            const QString c = m.captured(1).toUpper();
            if (hasUsdRateForSymbol(c)) {
                targetCurrency = c;
                expr = expr.left(m.capturedStart()).trimmed();
            }
        }
    }

    // Split into signed chunks on "  +  " or "  -  " (operator surrounded by spaces).
    // Leading unary sign is consumed first.
    QVector<QPair<int, QString>> chunks; // (sign, chunk_text)
    {
        int initialSign = 1;
        if (!expr.isEmpty() && expr[0] == QLatin1Char('-')) {
            initialSign = -1;
            expr = expr.mid(1).trimmed();
        } else if (!expr.isEmpty() && expr[0] == QLatin1Char('+')) {
            expr = expr.mid(1).trimmed();
        }

        static QRegularExpression opRx(R"((?<=[A-Za-z\d])\s*([+-])\s*(?=[A-Za-z\d(]))");
        QRegularExpressionMatchIterator it = opRx.globalMatch(expr);
        int start = 0;
        int nextSign = initialSign;
        QStringList parts;
        QVector<int> signs;

        while (it.hasNext()) {
            const auto m = it.next();
            parts.append(expr.mid(start, m.capturedStart() - start).trimmed());
            signs.append(nextSign);
            nextSign = (m.captured(1) == QLatin1String("-")) ? -1 : 1;
            start = m.capturedEnd();
        }
        parts.append(expr.mid(start).trimmed());
        signs.append(nextSign);

        for (int i = 0; i < parts.size(); ++i)
            if (!parts[i].isEmpty())
                chunks.append({signs[i], parts[i]});
    }

    if (chunks.isEmpty())
        return false;

    // Parse each chunk into (amount, currency).
    struct Term { double amount; QString currency; };
    QVector<QPair<int, Term>> terms;
    bool hasAnyCurrency = false;

    static QRegularExpression numCurrRx("^(\\d+(?:[.,]\\d+)?)\\s+([A-Za-z]{3,5})$");
    static QRegularExpression currNumRx("^([A-Za-z]{3,5})\\s+(\\d+(?:[.,]\\d+)?)$");
    static QRegularExpression plainNumRx("^([+-]?\\d+(?:[.,]\\d+)?)$");

    for (const auto &chunk : chunks) {
        const QString &s = chunk.second;
        Term t;

        // 1. Variable with currency tag
        if (m_varCurrencyTag.contains(s)) {
            t.amount   = m_varNumericValue.value(s, 0.0);
            t.currency = m_varCurrencyTag.value(s);
            hasAnyCurrency = true;
            terms.append({chunk.first, t});
            continue;
        }

        // 2. Variable without currency tag (plain numeric variable)
        if (m_varNumericValue.contains(s)) {
            t.amount   = m_varNumericValue.value(s);
            t.currency = QString();
            terms.append({chunk.first, t});
            continue;
        }

        // 3. NUMBER CURRENCY  (e.g. "200 USD", "1 BTC")
        {
            const auto m = numCurrRx.match(s);
            if (m.hasMatch()) {
                const QString curr = m.captured(2).toUpper();
                if (hasUsdRateForSymbol(curr)) {
                    QString n = m.captured(1);
                    n.replace(QLatin1Char(','), QLatin1Char('.'));
                    bool ok = false;
                    const double v = n.toDouble(&ok);
                    if (ok) {
                        t.amount = v;
                        t.currency = curr;
                        hasAnyCurrency = true;
                        terms.append({chunk.first, t});
                        continue;
                    }
                }
            }
        }

        // 4. CURRENCY NUMBER  (e.g. "USD 200", "BTC 1")
        {
            const auto m = currNumRx.match(s);
            if (m.hasMatch()) {
                const QString curr = m.captured(1).toUpper();
                if (hasUsdRateForSymbol(curr)) {
                    QString n = m.captured(2);
                    n.replace(QLatin1Char(','), QLatin1Char('.'));
                    bool ok = false;
                    const double v = n.toDouble(&ok);
                    if (ok) {
                        t.amount = v;
                        t.currency = curr;
                        hasAnyCurrency = true;
                        terms.append({chunk.first, t});
                        continue;
                    }
                }
            }
        }

        // 5. Plain NUMBER (dimensionless — inherits output currency)
        {
            const auto m = plainNumRx.match(s);
            if (m.hasMatch()) {
                QString n = m.captured(1);
                n.replace(QLatin1Char(','), QLatin1Char('.'));
                bool ok = false;
                const double v = n.toDouble(&ok);
                if (ok) {
                    t.amount = v;
                    t.currency = QString();
                    terms.append({chunk.first, t});
                    continue;
                }
            }
        }

        // Unrecognised chunk — not a currency expression we can handle
        return false;
    }

    // No currency context at all → let libqalculate handle it
    if (!hasAnyCurrency)
        return false;

    // Determine output currency:
    //   1. Explicit "to CURR" wins.
    //   2. When terms mix different currencies and the first tagged term is a
    //      cryptocurrency, fall back to m_defaultCurrency (e.g. "1 BTC + 1 ETH" → USD).
    //   3. Otherwise use the first tagged term's currency.
    QString outputCurrency = targetCurrency;
    if (outputCurrency.isEmpty()) {
        // Collect distinct currency tags across all terms
        QString firstCurrency;
        QSet<QString> distinctCurrencies;
        for (const auto &t : terms) {
            if (!t.second.currency.isEmpty()) {
                if (firstCurrency.isEmpty())
                    firstCurrency = t.second.currency;
                distinctCurrencies.insert(t.second.currency);
            }
        }

        if (firstCurrency.isEmpty())
            return false;

        // When terms mix different currencies, use the configured default output currency.
        if (distinctCurrencies.size() > 1
                && !m_defaultCurrency.isEmpty()
                && hasUsdRateForSymbol(m_defaultCurrency)) {
            outputCurrency = m_defaultCurrency;
        } else {
            outputCurrency = firstCurrency;
        }
    }
    if (outputCurrency.isEmpty())
        return false;

    bool outRateOk = false;
    const double outRate = usdRateForSymbol(outputCurrency, &outRateOk);
    if (!outRateOk || outRate <= 0.0)
        return false;

    // Sum all terms in output currency
    double total = 0.0;
    for (const auto &t : terms) {
        const double  amount = t.second.amount;
        const QString &curr  = t.second.currency;

        double inOutput;
        if (curr.isEmpty() || curr == outputCurrency) {
            inOutput = amount;
        } else {
            bool termOk = false;
            const double termRate = usdRateForSymbol(curr, &termOk);
            if (!termOk || termRate <= 0.0)
                return false;
            inOutput = amount * termRate / outRate;
        }
        total += t.first * inOutput;
    }

    *outResult = outputCurrency + QLatin1Char(' ') + smartFormat(total, m_decimalPlaces);
    if (outCurrency)
        *outCurrency = outputCurrency;
    return true;
}

QString QalcBridge::highlightLine(const QString &line) const
{
    return m_highlighter->highlightLine(line, {});
}

QList<LineResult> QalcBridge::evaluateDocument(const QString &source) {
    QMutexLocker locker(&m_calcMutex);

    // Clear per-document variable state from any previous run.
    m_varCurrencyTag.clear();
    m_varNumericValue.clear();

    // Clear user-defined variables from previous evaluations.
    // We collect names first to avoid iterator invalidation during removal.
    std::vector<std::string> toRemove;
    for (size_t i = 0; ; ++i) {
        Variable *v = m_calc->getVariable(i);
        if (!v) break;
        if (!v->isBuiltin()) {
            toRemove.push_back(v->name());
        }
    }
    for (const auto &name : toRemove) {
        m_calc->deleteName(name);
    }

    QList<LineResult> results;
    QStringList lines = source.split('\n');

    // Collect variable names for highlighting.
    QSet<QString> variables;
    static QRegularExpression assignmentRegex(
        "^([A-Za-z_π\\p{L}][\\wπ\\p{L}]*)\\s*(?::=|=)\\s*(.+)$",
        QRegularExpression::UseUnicodePropertiesOption);
    for (const QString &line : lines) {
        auto match = assignmentRegex.match(line.trimmed());
        if (match.hasMatch())
            variables.insert(match.captured(1));
    }

    EvaluationOptions eo;
    eo.parse_options.angle_unit = ANGLE_UNIT_RADIANS;
    eo.structuring = STRUCTURING_SIMPLIFY;
    eo.parse_options.unknowns_enabled = false;

    PrintOptions po;
    po.number_fraction_format = FRACTION_DECIMAL;
    po.base = 10;
    po.min_decimals = 0;
    po.max_decimals = m_decimalPlaces;
    po.use_min_decimals = false;
    po.use_max_decimals = true;
    po.digit_grouping = DIGIT_GROUPING_LOCALE;

    for (const QString &rawLine : lines) {
        LineResult res;
        res.ok = false;
        QString trimmed = rawLine.trimmed();
        const QString totalKey = totalKeyForExpression(trimmed);

        res.highlightedHtml = m_highlighter->highlightLine(rawLine, variables);

        // Empty / comment lines
        if (trimmed.isEmpty() || trimmed.startsWith("//") || trimmed.startsWith("#")) {
            res.ok = true;
            res.result = "";
            results.append(res);
            continue;
        }

        // Preprocess input for libqalculate compatibility
        QString line = rawLine;

        // Safely strip thousands separators from numbers.
        // e.g. "22,045.00" -> "22045.00".
        // We only strip if it's strictly a thousands separator between two digits.
        QLocale locale;
        QString groupSep = locale.groupSeparator();
        QString decimalPoint = locale.decimalPoint();
        if (!groupSep.isEmpty() && groupSep != decimalPoint) {
            for (int i = line.length() - (groupSep.length() + 1); i > 0; --i) {
                if (line.mid(i, groupSep.length()) == groupSep 
                    && line.at(i - 1).isDigit() 
                    && line.at(i + groupSep.length()).isDigit()) {
                    line.remove(i, groupSep.length());
                }
            }
        }

        // Support ':' as division operator (e.g. 2:4 -> 2/4)
        static QRegularExpression colonDivRegex("(?<=\\d)\\s*:\\s*(?=\\d)");
        line.replace(colonDivRegex, "/");

        // Unified currency evaluator: handles variables with currency tags, multi-currency
        // sums, cross-crypto arithmetic, and explicit conversions — all via our rate tables.
        // Must run before libqalculate to prevent it from choosing an arbitrary output unit.
        {
            QString currResult, currTag;
            if (tryEvaluateCurrencyExpr(line.trimmed(), &currResult, &currTag)) {
                res.ok = true;
                res.result = currResult;
                res.hasNumericValue = parseDisplayNumber(currResult, &res.numericValue);
                res.totalKey = totalKey;
                results.append(res);
                continue;
            }
        }

        // Numi-style arithmetic: "CURRENCY AMOUNT ± NUMBER" → compute directly.
        // We bypass libqalculate here because its print() converts currency
        // results to the base fiat currency (USD), losing the original unit.
        // E.g., "UAH 22045 - 600" → "UAH 21,445"
        static QRegularExpression currencyArithRegex(
            "^([A-Za-z]{3,5})\\s+(\\d+(?:[.,]\\d+)?)\\s*([+\\-])\\s*(\\d+(?:[.,]\\d+)?)$",
            QRegularExpression::CaseInsensitiveOption);
        {
            const auto cam = currencyArithRegex.match(line.trimmed());
            if (cam.hasMatch()) {
                const QString currency = cam.captured(1).toUpper();
                if (m_calc->getUnit(currency.toStdString())) {
                    QString n1str = cam.captured(2);
                    QString n2str = cam.captured(4);
                    n1str.replace(QLatin1Char(','), QLatin1Char('.'));
                    n2str.replace(QLatin1Char(','), QLatin1Char('.'));
                    bool ok1 = false, ok2 = false;
                    const double n1 = n1str.toDouble(&ok1);
                    const double n2 = n2str.toDouble(&ok2);
                    if (ok1 && ok2) {
                        const double result = (cam.captured(3) == QLatin1String("+")) ? n1 + n2 : n1 - n2;
                        res.ok = true;
                        res.result = currency + QLatin1Char(' ') + smartFormat(result, m_decimalPlaces);
                        res.hasNumericValue = true;
                        res.numericValue = result;
                        res.totalKey = totalKey;
                        results.append(res);
                        continue;
                    }
                }
            }
        }

        // Cross-currency arithmetic: "AMT1 CURR1 ± AMT2 CURR2"
        // Uses the same rate-via-USD approach as convertCurrencyExpression to avoid
        // libqalculate's print() converting results to the USD base currency.
        // E.g., "500 EUR - 100 USD" → rate(USD→EUR) → "EUR 415"
        static QRegularExpression crossCurrencyRegex(
            "^(\\d+(?:[.,]\\d+)?)\\s+([A-Za-z]{3,5})\\s*([+\\-])\\s*(\\d+(?:[.,]\\d+)?)\\s+([A-Za-z]{3,5})$",
            QRegularExpression::CaseInsensitiveOption);
        {
            const auto ccm = crossCurrencyRegex.match(line.trimmed());
            if (ccm.hasMatch()) {
                const QString curr1 = ccm.captured(2).toUpper();
                const QString curr2 = ccm.captured(5).toUpper();
                if (curr1 != curr2) {
                    QString n1str = ccm.captured(1); n1str.replace(QLatin1Char(','), QLatin1Char('.'));
                    QString n2str = ccm.captured(4); n2str.replace(QLatin1Char(','), QLatin1Char('.'));
                    bool ok1 = false, ok2 = false;
                    const double n1 = n1str.toDouble(&ok1);
                    const double n2 = n2str.toDouble(&ok2);
                    if (ok1 && ok2) {
                        bool r1ok = false, r2ok = false;
                        const double rate1 = usdRateForSymbol(curr1, &r1ok);
                        const double rate2 = usdRateForSymbol(curr2, &r2ok);
                        if (r1ok && r2ok && rate1 > 0.0) {
                            // n2 CURR2 = (n2 * rate2 / rate1) CURR1
                            const double n2InCurr1 = n2 * rate2 / rate1;
                            const double result = (ccm.captured(3) == QLatin1String("+"))
                                                  ? n1 + n2InCurr1 : n1 - n2InCurr1;
                            res.ok = true;
                            res.result = curr1 + QLatin1Char(' ') + smartFormat(result, m_decimalPlaces);
                            res.hasNumericValue = true;
                            res.numericValue = result;
                            res.totalKey = totalKey;
                            results.append(res);
                            continue;
                        }
                    }
                }
            }
        }

        if (isIncompleteExpression(trimmed)) {
            res.ok = true;
            res.result = "";
            results.append(res);
            continue;
        }

        if (hasExplicitDivisionByZero(trimmed)) {
            res.ok = false;
            res.error = "Error";
            results.append(res);
            continue;
        }

        if (trimmed.compare(QStringLiteral("time"), Qt::CaseInsensitive) == 0) {
            res.ok = true;
            res.result = QTime::currentTime().toString(QStringLiteral("HH:mm:ss"));
            results.append(res);
            continue;
        }

        if (trimmed.compare(QStringLiteral("now"), Qt::CaseInsensitive) == 0) {
            res.ok = true;
            res.result = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
            results.append(res);
            continue;
        }

        QString dateDifferenceResult;
        if (tryDateDifference(trimmed, &dateDifferenceResult)) {
            res.ok = true;
            res.result = dateDifferenceResult;
            results.append(res);
            continue;
        }

        QString dateArithmeticResult;
        if (tryDateArithmetic(trimmed, &dateArithmeticResult)) {
            res.ok = true;
            res.result = dateArithmeticResult;
            results.append(res);
            continue;
        }

        bool currencyConverted = false;
        const QString currencyResult = convertCurrencyExpression(line, &currencyConverted);
        if (currencyConverted) {
            res.ok = true;
            res.result = currencyResult;
            res.hasNumericValue = parseDisplayNumber(currencyResult, &res.numericValue);
            res.totalKey = totalKey;
            results.append(res);
            continue;
        }

        // 1. "now" → "today()"
        static QRegularExpression nowRegex("\\bnow\\b", QRegularExpression::CaseInsensitiveOption);
        line.replace(nowRegex, "today()");

        // 2. Case-insensitive currencies: uppercase standalone 3-letter words
        //    Avoids converting natural-language words (to, in, of, as, from)
        static QSet<QString> skipWords = {"the", "and", "but", "not", "for", "nor",
                                           "yet", "has", "had", "was", "are", "its"};
        static QRegularExpression currencyRegex("\\b([a-z]{3,5})\\b");
        
        auto preprocessCurrencies = [&](QString l) -> QString {
            QRegularExpressionMatchIterator currIt = currencyRegex.globalMatch(l);
            struct Replacement { qsizetype start; qsizetype len; QString replacement; };
            QList<Replacement> reps;
            while (currIt.hasNext()) {
                auto m = currIt.next();
                QString word = m.captured(1);
                QString upper = word.toUpper();
                if (!skipWords.contains(word) && m_calc->getUnit(upper.toStdString())) {
                    reps.prepend({m.capturedStart(1), m.capturedLength(1), upper});
                }
            }
            for (const auto &r : reps)
                l.replace(r.start, r.len, r.replacement);
            return l;
        };

        line = preprocessCurrencies(line);

        // 3. Percentage patterns: "X% from Y" / "X% of Y" → "(X/100)*Y"
        //    Applied to RHS for assignments, to the full line otherwise.
        static QRegularExpression percentFromOf(
            "([\\d.]+)\\s*%\\s+(?:from|of)\\s+(.+)$",
            QRegularExpression::CaseInsensitiveOption);


        auto applyPercentPreprocess = [&](QString expr) -> QString {
            auto pfm = percentFromOf.match(expr.trimmed());
            if (pfm.hasMatch())
                return QString("(%1/100)*%2").arg(pfm.captured(1), pfm.captured(2));
            return expr;
        };

        // 4. Variable assignment: detect first, then preprocess RHS
        auto assignMatch = assignmentRegex.match(line.trimmed());
        if (assignMatch.hasMatch()) {
            QString varName = assignMatch.captured(1);
            
            // Validation: prevent using keywords as variable names.
            // We allow shadowing units (like 'A' for Ampere) as it's common in calculations.
            if (isIncompleteExpression(varName)) {
                res.ok = false;
                res.error = QStringLiteral("Reserved name: %1").arg(varName);
                results.append(res);
                continue;
            }

            QString rhs = applyPercentPreprocess(assignMatch.captured(2));

            // Try currency evaluation on the RHS before libqalculate.
            // This handles "VAR = N CURR to CURR2", "VAR = N CURR1 + M CURR2", etc.
            {
                QString currResult, currTag;
                if (tryEvaluateCurrencyExpr(rhs, &currResult, &currTag)) {
                    double numVal = 0.0;
                    parseDisplayNumber(currResult, &numVal);
                    if (std::isfinite(numVal)) {
                        m_varCurrencyTag[varName]  = currTag;
                        m_varNumericValue[varName] = numVal;
                        // Store as plain number in libqalculate so arithmetic with
                        // other variables continues to work.
                        const QString qa = QStringLiteral("%1 := %2")
                                            .arg(varName)
                                            .arg(numVal, 0, 'g', 15);
                        MathStructure tmp;
                        m_calc->calculate(&tmp, qa.toStdString(), 5000, eo);
                        res.ok = true;
                        res.result = currResult;
                        res.hasNumericValue = true;
                        res.numericValue = numVal;
                        res.totalKey = totalKey;
                        results.append(res);
                        continue;
                    }
                }
            }

            try {
                m_calc->clearMessages();
                MathStructure rhsResult;
                m_calc->calculate(&rhsResult, rhs.toStdString(), 5000, eo);
                if (m_calc->aborted() || hasCalculationError(m_calc) || rhsResult.isUndefined() || rhsResult.isInfinite()) {
                    res.ok = false;
                    res.error = "Error";
                } else {
                    // Set the variable in the calculator.
                    // We use an internal assignment expression to let libqalculate handle the variable update properly.
                    QString assignment = QString("%1 := %2").arg(varName, rhs);
                    MathStructure _assignTmp;
                    m_calc->calculate(&_assignTmp, assignment.toStdString(), 5000, eo);

                    if (rhsResult.isNumber()) {
                        double v = rhsResult.number().floatValue();
                        if (!std::isfinite(v)) {
                            res.ok = false;
                            res.error = "Error";
                        } else {
                            m_varNumericValue[varName] = v;
                            res.result = smartFormat(v, m_decimalPlaces);
                            res.hasNumericValue = true;
                            res.numericValue = v;
                            res.totalKey = totalKey;
                            res.ok = true;
                        }
                    } else {
                        QString rhsStr = QString::fromStdString(m_calc->print(rhsResult, 2000, po));
                        res.result = rhsStr;
                        res.ok = !hasCalculationError(m_calc);
                        if (!res.ok) {
                            res.error = "Error";
                        } else {
                            // Re-assign as plain number so downstream arithmetic works.
                            // Currency-bearing RHS expressions are now intercepted above by
                            // tryEvaluateCurrencyExpr; this branch handles non-currency
                            // non-scalar results from libqalculate (e.g. unit conversions
                            // that libqalculate resolved on its own).
                            double numericForVar = 0.0;
                            if (parseDisplayNumber(rhsStr, &numericForVar) && std::isfinite(numericForVar)) {
                                m_varNumericValue[varName] = numericForVar;
                                QString numAssignment = QString("%1 := %2").arg(varName).arg(numericForVar, 0, 'g', 15);
                                MathStructure _numTmp;
                                m_calc->calculate(&_numTmp, numAssignment.toStdString(), 5000, eo);
                                res.hasNumericValue = true;
                                res.numericValue = numericForVar;
                                res.totalKey = totalKey;
                            }
                        }
                    }
                }
            } catch (...) {
                res.ok = false;
                res.error = "Error";
            }
        } else {
            // "X% from/of NUMBER CURRENCY" — libqalculate always normalises
            // currency results to USD when multiplying by a dimensionless fraction,
            // so compute this pattern directly to preserve the original currency.
            static QRegularExpression percentOfCurrencyRx(
                "^([+-]?\\d+(?:[.,]\\d+)?)\\s*%\\s+(?:from|of)\\s+(\\d+(?:[.,]\\d+)?)\\s+([A-Za-z]{3,5})$",
                QRegularExpression::CaseInsensitiveOption);
            const auto pocm = percentOfCurrencyRx.match(line.trimmed());
            if (pocm.hasMatch()) {
                bool okP, okN;
                const double pct = pocm.captured(1).toDouble(&okP);
                QString numStr = pocm.captured(2);
                numStr.replace(QLatin1Char(','), QLatin1Char('.'));
                const double num = numStr.toDouble(&okN);
                if (okP && okN) {
                    const double value = pct / 100.0 * num;
                    res.ok = true;
                    res.result = pocm.captured(3).toUpper() + QLatin1Char(' ')
                                 + smartFormat(value, m_decimalPlaces);
                    res.hasNumericValue = true;
                    res.numericValue = value;
                    res.totalKey = totalKey;
                    results.append(res);
                    continue;
                }
            }

            line = applyPercentPreprocess(line);
            try {
                m_calc->clearMessages();
                MathStructure result;
                m_calc->calculate(&result, line.toStdString(), 5000, eo);
                if (m_calc->aborted() || hasCalculationError(m_calc) || result.isUndefined() || result.isInfinite()) {
                    res.ok = false;
                    res.error = "Error";
                } else if (result.isNumber()) {
                    double v = result.number().floatValue();
                    if (!std::isfinite(v)) {
                        res.ok = false;
                        res.error = "Error";
                    } else {
                        res.result = smartFormat(v, m_decimalPlaces);
                        res.hasNumericValue = true;
                        res.numericValue = v;
                        res.totalKey = totalKey;
                        res.ok = true;
                    }
                } else {
                    MathStructure resVal = result;
                    resVal.expand(eo);
                    QString out = QString::fromStdString(m_calc->print(resVal, 2000, po));
                    // Strip surrounding quotes (dates, strings)
                    if (out.startsWith('"') && out.endsWith('"'))
                        out = out.mid(1, out.length() - 2);

                    // Disallow vectors/matrices. 
                    // Previously we also disallowed " + " and " - " to hide unsimplified results,
                    // but this broke currency sums (e.g. 600 AED + 400 USD).
                    if (out.startsWith('[') && out.endsWith(']')) {
                        res.ok = false;
                        res.error = "Error";
                    } else {
                        res.result = out;
                        static QRegularExpression conversionWord(
                            "\\b(?:to|in|as)\\b",
                            QRegularExpression::CaseInsensitiveOption);
                        if (conversionWord.match(trimmed).hasMatch())
                            res.hasNumericValue = parseDisplayNumber(out, &res.numericValue);
                        if (res.hasNumericValue)
                            res.totalKey = totalKey;
                        res.ok = !hasCalculationError(m_calc);
                    }
                    if (!res.ok) res.error = "Error";
                }
            } catch (...) {
                res.ok = false;
                res.error = "Error";
            }
        }

        if (res.ok && !res.hasNumericValue && !res.result.isEmpty()) {
            bool containsDigits = trimmed.contains(QRegularExpression("\\d"));
            bool isAssignment = trimmed.contains('=');
            bool hasOperators = trimmed.contains(QRegularExpression("[+\\-*/^%]"));
            bool isKnownSymbol = m_calc->getVariable(trimmed.toStdString()) 
                              || m_calc->getUnit(trimmed.toStdString())
                              || m_calc->getFunction(trimmed.toStdString());
            
            if (!containsDigits && !isAssignment && !hasOperators && !isKnownSymbol) {
                res.ok = true;
                res.result = "";
                res.error = "";
            }
        }

        if (!res.ok) {
            // If the line fails and contains no digits, treat it as junk text/comment and hide result
            if (!trimmed.contains(QRegularExpression("\\d"))) {
                res.ok = true;
                res.result = "";
                res.error = "";
            } else {
                if (res.error.isEmpty())
                    res.error = "Error";
            }
        }

        results.append(res);
    }

    return results;
}

QString QalcBridge::getCompletion(const QString &prefix) {
    if (prefix.isEmpty()) return prefix;

    QStringList matches;
    std::string p = prefix.toLower().toStdString();

    for (size_t i = 0; ; ++i) {
        Unit *u = m_calc->getUnit(i);
        if (!u) break;
        for (size_t j = 0; j < u->countNames(); ++j) {
            const std::string &name = u->getName(j).name;
            if (name.find(p) == 0) matches << QString::fromStdString(name);
        }
    }
    for (size_t i = 0; ; ++i) {
        Variable *v = m_calc->getVariable(i);
        if (!v) break;
        for (size_t j = 0; j < v->countNames(); ++j) {
            const std::string &name = v->getName(j).name;
            if (name.find(p) == 0) matches << QString::fromStdString(name);
        }
    }
    for (size_t i = 0; ; ++i) {
        MathFunction *f = m_calc->getFunction(i);
        if (!f) break;
        for (size_t j = 0; j < f->countNames(); ++j) {
            const std::string &name = f->getName(j).name;
            if (name.find(p) == 0) matches << QString::fromStdString(name);
        }
    }

    static const QStringList keywords = {
        "today", "tomorrow", "yesterday", "now",
        "days", "weeks", "months", "years"
    };
    for (const auto &kw : keywords) {
        if (kw.startsWith(prefix, Qt::CaseInsensitive)) matches << kw;
    }

    matches.removeDuplicates();
    if (matches.isEmpty()) return prefix;
    if (matches.size() == 1) return matches.first();

    QString common = matches.first();
    for (int i = 1; i < matches.size(); ++i) {
        const QString &s = matches.at(i);
        int j = 0;
        while (j < common.length() && j < s.length() &&
               common[j].toLower() == s[j].toLower())
            j++;
        common = common.left(j);
    }
    return common.isEmpty() ? prefix : common;
}
