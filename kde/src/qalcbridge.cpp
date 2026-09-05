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
#include <QDebug>
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
#include <QDir>
#include <QFile>
#include <QStandardPaths>
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
    loadRatesFromCache();

    // Fetch on startup, then refresh every hour.
    m_ratesRefreshTimer = new QTimer(this);
    m_ratesRefreshTimer->setInterval(60 * 60 * 1000);
    connect(m_ratesRefreshTimer, &QTimer::timeout, this, [this]() {
        fetchFiatRates();
        fetchCryptoRates();
    });

    // Tests set NUMI_KDE_OFFLINE_RATES: rates come only from the cache file
    // (a fixture), so results never depend on the network or live prices.
    if (qEnvironmentVariableIsSet("NUMI_KDE_OFFLINE_RATES"))
        return;

    m_ratesRefreshTimer->start();
    fetchFiatRates();
    fetchCryptoRates();
}

QalcBridge::~QalcBridge() {
    // Calculator's destructor joins its calculation thread; make sure that
    // thread is not in the middle of a long computation.
    abortCalculation();
    delete m_highlighter;
    delete m_calc;
}

QalcBridge::NetworkStatus QalcBridge::networkStatus() const
{
    if (m_fiatStatus == NetworkStatus::Fetching || m_cryptoStatus == NetworkStatus::Fetching)
        return NetworkStatus::Fetching;
    if (m_fiatStatus == NetworkStatus::Success && m_cryptoStatus == NetworkStatus::Success)
        return NetworkStatus::Success;
    if (m_fiatStatus == NetworkStatus::Idle && m_cryptoStatus == NetworkStatus::Idle)
        return NetworkStatus::Idle;
    if (m_fiatStatus == NetworkStatus::Error && m_cryptoStatus == NetworkStatus::Error)
        return NetworkStatus::Error;
    return NetworkStatus::Partial;
}

void QalcBridge::fetchFiatRates() {
    m_fiatStatus = NetworkStatus::Fetching;
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
    m_cryptoStatus = NetworkStatus::Fetching;
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
        m_fiatStatus = NetworkStatus::Error;
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
        saveRatesToCache(false, usdRates);
        m_fiatStatus = NetworkStatus::Success;
        emit networkStatusChanged();
        emit ratesUpdated();
    } else {
        qWarning() << "numi-kde: fiat rates response contained no usable rates (unexpected API format)";
        m_fiatStatus = NetworkStatus::Error;
        emit networkStatusChanged();
    }
}

void QalcBridge::onCryptoReply(QNetworkReply *reply) {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        m_cryptoStatus = NetworkStatus::Error;
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
        saveRatesToCache(true, rates);
        m_cryptoStatus = NetworkStatus::Success;
        emit networkStatusChanged();
        emit ratesUpdated();
    } else {
        qWarning() << "numi-kde: crypto rates response contained no usable rates (unexpected API format or all prices zero)";
        m_cryptoStatus = NetworkStatus::Error;
        emit networkStatusChanged();
    }
}

// If a currency unit named `name` already exists and is one of our alias units
// based directly on USD, refresh its rate in place. Returns false when the unit
// is missing or is libqalculate's own (EUR-based) definition, in which case the
// caller replaces it. Avoids leaking a fresh AliasUnit on every hourly refresh.
bool QalcBridge::updateUsdAliasUnit(const QString &name, double usdPerUnit, Unit *usd) {
    Unit *existing = m_calc->getUnit(name.toStdString());
    if (!existing || existing->subtype() != SUBTYPE_ALIAS_UNIT)
        return false;
    auto *alias = static_cast<AliasUnit *>(existing);
    if (alias->firstBaseUnit() != usd || alias->isBuiltin())
        return false;
    alias->setExpression(std::to_string(usdPerUnit));
    return true;
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

        if (updateUsdAliasUnit(normalized, usdPerCurrency, usd))
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

        if (updateUsdAliasUnit(symbol, price, usd))
            continue;

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
    QMutexLocker locker(&m_calcMutex);
    m_decimalPlaces = places;
}

void QalcBridge::abortCalculation() {
    // Safe to call from any thread. The flag makes evaluateDocument() skip the
    // lines it has not reached yet; libqalculate's abort() interrupts the line
    // being calculated right now (it only acts while a timed calculate() runs).
    m_abortRequested.store(true);
    if (m_calc->busy())
        m_calc->abort();
}

void QalcBridge::setDefaultCurrency(const QString &currency) {
    QMutexLocker locker(&m_calcMutex);
    const QString upper = currency.trimmed().toUpper();
    m_defaultCurrency = upper.isEmpty() ? QStringLiteral("USD") : upper;
}

// Format a number with exactly maxDecimals decimal places.
// Uses system locale for thousands separators and decimal point.
static QString smartFormat(double value, int maxDecimals) {
    if (std::isinf(value) || std::isnan(value)) return QString::number(value);
    QLocale locale;
    if (maxDecimals == 0) return locale.toString(value, 'f', 0);
    QString s = locale.toString(value, 'f', maxDecimals);
    const QString dec = locale.decimalPoint();
    if (s.contains(dec)) {
        while (s.endsWith(QLatin1Char('0'))) s.chop(1);
        if (s.endsWith(dec)) s.chop(1);
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
    static const QStringList formatsFullYear = {
        QStringLiteral("d.M.yyyy"),
        QStringLiteral("dd.MM.yyyy"),
        QStringLiteral("d/M/yyyy"),
        QStringLiteral("dd/MM/yyyy"),
        QStringLiteral("d-M-yyyy"),
        QStringLiteral("dd-MM-yyyy"),
        QStringLiteral("yyyy-MM-dd"),
    };
    // Qt6 maps yy → 1900–1999; add 100 years to get 2000–2099
    static const QStringList formatsShortYear = {
        QStringLiteral("d.M.yy"),
        QStringLiteral("dd.MM.yy"),
        QStringLiteral("d/M/yy"),
        QStringLiteral("dd/MM/yy"),
        QStringLiteral("d-M-yy"),
        QStringLiteral("dd-MM-yy"),
    };

    const QString trimmed = value.trimmed();

    for (const QString &format : formatsFullYear) {
        const QDate date = QDate::fromString(trimmed, format);
        if (date.isValid()) return date;
    }
    for (const QString &format : formatsShortYear) {
        const QDate date = QDate::fromString(trimmed, format);
        if (date.isValid()) return date.addYears(100);
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

static bool tryTemperatureConversion(const QString &trimmed, int decimalPlaces, QString *result)
{
    // Matches "NUMBER C|F|K to C|F|K" (with optional leading ° or º symbol).
    // C = Celsius, F = Fahrenheit, K = Kelvin.
    // We handle this ourselves because libqalculate uses C = Coulomb and F = Farad.
    static QRegularExpression tempRx(
        "^\\s*([-+]?\\d+(?:[.,]\\d+)?)\\s*[°º]?([CFK])\\s+to\\s+[°º]?([CFK])\\s*$",
        QRegularExpression::CaseInsensitiveOption);
    const auto match = tempRx.match(trimmed);
    if (!match.hasMatch()) return false;

    const QString fromUnit = match.captured(2).toUpper();
    const QString toUnit   = match.captured(3).toUpper();
    if (fromUnit == toUnit) return false;

    QString amountStr = match.captured(1);
    amountStr.replace(QLatin1Char(','), QLatin1Char('.'));
    bool ok = false;
    const double value = amountStr.toDouble(&ok);
    if (!ok) return false;

    // Convert to Celsius as intermediate
    double celsius;
    if (fromUnit == QStringLiteral("C"))      celsius = value;
    else if (fromUnit == QStringLiteral("F")) celsius = (value - 32.0) * 5.0 / 9.0;
    else                                       celsius = value - 273.15;  // K

    // Physical lower bound: absolute zero = 0 K = −273.15 °C.
    // Return true with empty result to signal a physics error to the caller
    // (prevents libqalculate from returning a nonsensical fallback).
    if (celsius < -273.15) {
        *result = QString();
        return true;
    }

    double output;
    QString unitLabel;
    if (toUnit == QStringLiteral("C"))      { output = celsius;                       unitLabel = QStringLiteral("°C"); }
    else if (toUnit == QStringLiteral("F")) { output = celsius * 9.0 / 5.0 + 32.0;  unitLabel = QStringLiteral("°F"); }
    else                                    { output = celsius + 273.15;              unitLabel = QStringLiteral("K");  }

    *result = smartFormat(output, decimalPlaces) + QLatin1Char(' ') + unitLabel;
    return true;
}

static bool tryTimeArithmetic(const QString &trimmed, QString *result)
{
    static QRegularExpression timeArithRx(
        "^\\s*(?:time|now)\\s*([+-])\\s*(\\d+(?:[.,]\\d+)?)\\s*(s|sec|second|seconds|min|minute|minutes|h|hour|hours)\\s*$",
        QRegularExpression::CaseInsensitiveOption);
    const auto match = timeArithRx.match(trimmed);
    if (!match.hasMatch()) return false;
    const int sign = (match.captured(1) == QStringLiteral("+")) ? 1 : -1;
    QString amountStr = match.captured(2);
    amountStr.replace(QLatin1Char(','), QLatin1Char('.'));
    const double amount = amountStr.toDouble();
    const QString unit = match.captured(3).toLower();
    int secs = 0;
    if (unit == QStringLiteral("s") || unit == QStringLiteral("sec")
            || unit == QStringLiteral("second") || unit == QStringLiteral("seconds"))
        secs = qRound(amount);
    else if (unit == QStringLiteral("min") || unit == QStringLiteral("minute")
             || unit == QStringLiteral("minutes"))
        secs = qRound(amount * 60.0);
    else if (unit == QStringLiteral("h") || unit == QStringLiteral("hour")
             || unit == QStringLiteral("hours"))
        secs = qRound(amount * 3600.0);
    else
        return false;
    *result = QTime::currentTime().addSecs(sign * secs).toString(QStringLiteral("HH:mm:ss"));
    return true;
}

static bool tryDateArithmetic(const QString &trimmed, QString *result)
{
    static QRegularExpression dateMath(
        "^\\s*(\\d{1,4}[./-]\\d{1,2}[./-]\\d{1,4})\\s*([+-])\\s*(\\d+)\\s*(day|days|week|weeks|month|months|year|years)\\s*$",
        QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression namedDateMath(
        "^\\s*(today|now|tomorrow|yesterday)\\s*([+-])\\s*(\\d+)\\s*(day|days|week|weeks|month|months|year|years)\\s*$",
        QRegularExpression::CaseInsensitiveOption);

    QDate date;
    QString op, amountStr, unit;

    const auto namedMatch = namedDateMath.match(trimmed);
    if (namedMatch.hasMatch()) {
        const QString keyword = namedMatch.captured(1).toLower();
        if (keyword == QStringLiteral("tomorrow"))
            date = QDate::currentDate().addDays(1);
        else if (keyword == QStringLiteral("yesterday"))
            date = QDate::currentDate().addDays(-1);
        else
            date = QDate::currentDate();
        op = namedMatch.captured(2);
        amountStr = namedMatch.captured(3);
        unit = namedMatch.captured(4).toLower();
    } else {
        const auto match = dateMath.match(trimmed);
        if (!match.hasMatch())
            return false;
        date = parseUserDate(match.captured(1));
        if (!date.isValid())
            return false;
        op = match.captured(2);
        amountStr = match.captured(3);
        unit = match.captured(4).toLower();
    }

    int amount = amountStr.toInt();
    if (op == QStringLiteral("-"))
        amount = -amount;

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
        "^\\s*([+-]?(?:\\d+(?:\\.\\d+)?|\\.\\d+)(?:[eE][+-]?\\d+)?)\\s+([A-Za-z]{2,5})\\s+to\\s+([A-Za-z]{2,5})\\s*$",
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

        // (?<![eE]) prevents the exponent sign in scientific notation (e.g. "1e-3") from
        // being parsed as an arithmetic operator.
        static QRegularExpression opRx(R"((?<=[A-Za-z\d])(?<![eE])\s*([+-])\s*(?=[A-Za-z\d(]))");
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

    static QRegularExpression numCurrRx("^(\\d+(?:[.,]\\d+)?(?:[eE][+-]?\\d+)?)\\s*([A-Za-z]{3,5})$");
    static QRegularExpression currNumRx("^([A-Za-z]{3,5})\\s*(\\d+(?:[.,]\\d+)?(?:[eE][+-]?\\d+)?)$");
    static QRegularExpression plainNumRx("^([+-]?\\d+(?:[.,]\\d+)?(?:[eE][+-]?\\d+)?)$");

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
    QMutexLocker locker(&m_calcMutex);
    return m_highlighter->highlightLine(line, {}, {});
}

QList<LineResult> QalcBridge::evaluateDocument(const QString &source) {
    QMutexLocker locker(&m_calcMutex);
    m_abortRequested.store(false);

    // Clear per-document variable state from any previous run.
    m_varCurrencyTag.clear();
    m_varNumericValue.clear();
    m_displayToInternal.clear();
    m_internalToDisplay.clear();
    m_userVarDisplayValues.clear();

    // Clear user-defined variables from previous evaluations.
    // We collect names first to avoid iterator invalidation during removal.
    // Only *local* variables are ours: the global definitions also contain
    // non-builtin unknowns such as x, y, z, and deleting those left dangling
    // pointers inside libqalculate — `sum(sin(x), 1, 10, x)` crashed the app.
    std::vector<std::string> toRemove;
    for (size_t i = 0; ; ++i) {
        Variable *v = m_calc->getVariable(i);
        if (!v) break;
        if (v->isLocal() && !v->isBuiltin()) {
            toRemove.push_back(v->name());
        }
    }
    for (const auto &name : toRemove) {
        m_calc->deleteName(name);
    }

    QList<LineResult> results;
    QStringList lines = source.split('\n');

    // Pre-scan: collect all user variable display names and build display↔internal maps.
    // Multi-word assignment regex: LHS can contain unicode letters, digits, underscores and spaces.
    static QRegularExpression assignmentRegex(
        "^([\\p{L}_][\\p{L}\\d_ ]*?)\\s*(?::=|=)\\s*(.+)$",
        QRegularExpression::UseUnicodePropertiesOption);
    // Looser regex for pre-scan: detects variable name even if RHS is empty ("VarName =")
    static QRegularExpression varNameRegex(
        "^([\\p{L}_][\\p{L}\\d_ ]*?)\\s*(?::=|=)",
        QRegularExpression::UseUnicodePropertiesOption);
    QSet<QString> conflictingInternalNames;
    for (const QString &rawLn : lines) {
        QString t = rawLn.trimmed();
        if (t.startsWith('#') || t.isEmpty()) continue;
        auto m = varNameRegex.match(t);
        if (!m.hasMatch()) continue;
        QString displayName = m.captured(1).trimmed();
        if (displayName.isEmpty()) continue;
        QString internalName = displayName;
        internalName.replace(QRegularExpression("\\s+"), "_");
        if (!m_displayToInternal.contains(displayName)) {
            if (m_internalToDisplay.contains(internalName)) {
                // Two different display names map to the same internal name.
                // E.g. "monthly income" and "monthly_income" both become "monthly_income".
                conflictingInternalNames.insert(internalName);
            } else {
                m_displayToInternal.insert(displayName, internalName);
                m_internalToDisplay.insert(internalName, displayName);
            }
        }
    }

    // Build sorted lists for syntax highlighting (longest names first to avoid partial matches).
    QSet<QString> singleWordVars;
    QStringList multiWordVarsSorted;
    for (auto it = m_displayToInternal.constBegin(); it != m_displayToInternal.constEnd(); ++it) {
        if (it.key().contains(' '))
            multiWordVarsSorted << it.key();
        else
            singleWordVars << it.key();
    }
    std::sort(multiWordVarsSorted.begin(), multiWordVarsSorted.end(),
              [](const QString &a, const QString &b) { return a.length() > b.length(); });

    // Sorted display names for substituteVars (multi-word only, longest first).
    QStringList multiWordDisplayNames = multiWordVarsSorted;

    EvaluationOptions eo;
    eo.parse_options.angle_unit = ANGLE_UNIT_RADIANS;
    eo.structuring = STRUCTURING_SIMPLIFY;
    eo.parse_options.unknowns_enabled = false;

    PrintOptions po;
    po.number_fraction_format = FRACTION_DECIMAL;
    po.base = 10;
    po.min_decimals = m_decimalPlaces;
    po.max_decimals = m_decimalPlaces;
    po.use_min_decimals = true;
    po.use_max_decimals = true;
    po.digit_grouping = DIGIT_GROUPING_LOCALE;

    // For compound unit expressions (non-numeric libqalculate results): don't force
    // trailing zeros — "4 min + 20 s" is cleaner than "4.00 min + 20.00 s".
    PrintOptions po_display = po;
    po_display.min_decimals = 0;

    // Replaces multi-word display names with their underscore-normalised internal names
    // so libqalculate can handle them. Single-word names are stored as-is and need no substitution.
    auto substituteVars = [&](const QString &text) -> QString {
        QString result = text;
        for (const QString &disp : multiWordDisplayNames)
            result.replace(disp, m_displayToInternal.value(disp));
        return result;
    };

    for (const QString &rawLine : lines) {
        LineResult res;
        res.ok = false;
        QString trimmed = rawLine.trimmed();
        const QString totalKey = totalKeyForExpression(trimmed);

        res.highlightedHtml = m_highlighter->highlightLine(rawLine, singleWordVars, multiWordVarsSorted);

        // The document was superseded (new input arrived): stop spending time
        // on the remaining lines, the caller discards this result set anyway.
        if (m_abortRequested.load()) {
            res.ok = true;
            results.append(res);
            continue;
        }

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
        // e.g. "22,045.00" -> "22045.00", "1,000,000" -> "1000000".
        // Only matches real thousands groups: digit(s) followed by (sep + exactly 3 digits)+.
        // "10,5" is NOT a thousands separator (only 1 digit after comma) and is left intact
        // so that European decimal notation like "10,5% of 200" works correctly.
        QLocale locale;
        const QString groupSep = locale.groupSeparator();
        const QString decimalPoint = locale.decimalPoint();
        if (!groupSep.isEmpty() && groupSep != decimalPoint) {
            const QRegularExpression thousandsRe(
                "\\d{1,3}(?:" + QRegularExpression::escape(groupSep) + "\\d{3})+");
            QRegularExpressionMatchIterator it = thousandsRe.globalMatch(line);
            QList<QPair<int,int>> spans;
            while (it.hasNext()) {
                const auto m = it.next();
                spans.prepend({m.capturedStart(), m.capturedLength()});
            }
            for (const auto &span : spans) {
                QString num = line.mid(span.first, span.second);
                num.remove(groupSep);
                line.replace(span.first, span.second, num);
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
                res.totalKey = currTag.isEmpty() ? totalKey : currTag;
                results.append(res);
                continue;
            }
        }

        // Numi-style arithmetic: "CURRENCY AMOUNT ± NUMBER" → compute directly.
        // We bypass libqalculate here because its print() converts currency
        // results to the base fiat currency (USD), losing the original unit.
        // E.g., "UAH 22045 - 600" → "UAH 21,445"
        static QRegularExpression currencyArithRegex(
            "^([A-Za-z]{3,5})\\s+(\\d+(?:[.,]\\d+)?(?:[eE][+-]?\\d+)?)\\s*([+\\-])\\s*(\\d+(?:[.,]\\d+)?(?:[eE][+-]?\\d+)?)$",
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
                        res.totalKey = currency;
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
            "^(\\d+(?:[.,]\\d+)?(?:[eE][+-]?\\d+)?)\\s+([A-Za-z]{3,5})\\s*([+\\-])\\s*(\\d+(?:[.,]\\d+)?(?:[eE][+-]?\\d+)?)\\s+([A-Za-z]{3,5})$",
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
                            res.totalKey = curr1;
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

        {
            QString timeArithResult;
            if (tryTimeArithmetic(trimmed, &timeArithResult)) {
                res.ok = true;
                res.result = timeArithResult;
                results.append(res);
                continue;
            }
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

        {
            QString tempResult;
            if (tryTemperatureConversion(trimmed, m_decimalPlaces, &tempResult)) {
                if (tempResult.isEmpty()) {
                    // Physics violation (e.g. below absolute zero)
                    res.ok = false;
                    res.error = QStringLiteral("Error");
                } else {
                    res.ok = true;
                    res.result = tempResult;
                    res.hasNumericValue = parseDisplayNumber(tempResult, &res.numericValue);
                }
                results.append(res);
                continue;
            }
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
            "([\\d.,]+)\\s*%\\s+(?:from|of)\\s+(.+)$",
            QRegularExpression::CaseInsensitiveOption);

        auto applyPercentPreprocess = [&](QString expr) -> QString {
            auto pfm = percentFromOf.match(expr.trimmed());
            if (!pfm.hasMatch())
                return expr;
            QString numStr = pfm.captured(1);
            numStr.replace(QLatin1Char(','), QLatin1Char('.'));
            bool ok = false;
            const double pct = numStr.toDouble(&ok);
            if (!ok) return expr;
            // Build an integer fraction to avoid locale-sensitive decimal point
            // interpretation by libqalculate (some locales treat "." as thousands separator).
            // E.g. "10.5" → numerator=105, scale=10 → "(105/1000)*RHS" instead of "(10.5/100)*RHS".
            const int dotPos = numStr.indexOf(QLatin1Char('.'));
            const int decDigits = (dotPos >= 0) ? (numStr.length() - dotPos - 1) : 0;
            qint64 scale = 1;
            for (int i = 0; i < decDigits; ++i) scale *= 10;
            const qint64 numerator = qRound64(pct * static_cast<double>(scale));
            return QString("(%1/%2)*%3").arg(numerator).arg(scale * 100LL).arg(pfm.captured(2));
        };

        // 4. Variable assignment: detect first, then preprocess RHS
        auto assignMatch = assignmentRegex.match(line.trimmed());
        if (assignMatch.hasMatch()) {
            QString displayName = assignMatch.captured(1).trimmed();
            // For names not in the map (skipped due to collision), normalise the same way.
            QString internalName = m_displayToInternal.contains(displayName)
                ? m_displayToInternal.value(displayName)
                : QString(displayName).replace(QRegularExpression("\\s+"), "_");

            // Detect collision: two different display names map to the same internal name.
            if (conflictingInternalNames.contains(internalName)) {
                res.ok = false;
                res.error = QStringLiteral("Variable name conflict: \"%1\" clashes with another variable name").arg(displayName);
                results.append(res);
                continue;
            }

            // Validation: prevent using keywords as variable names.
            // We allow shadowing units (like 'A' for Ampere) as it's common in calculations.
            if (isIncompleteExpression(displayName)) {
                res.ok = false;
                res.error = QStringLiteral("Reserved name: %1").arg(displayName);
                results.append(res);
                continue;
            }

            // Strip "(annotation) = value" prefix from RHS — Numi syntax for labeled assignments.
            // e.g., "X = (label) = 100 + 200": (label) is an annotation, 100+200 is the actual value.
            static QRegularExpression annotationPrefixRe(R"(^\(.*?\)\s*[=:]=?\s*(.+)$)");
            QString rawRhs = assignMatch.captured(2).trimmed();
            auto annM = annotationPrefixRe.match(rawRhs);
            if (annM.hasMatch())
                rawRhs = annM.captured(1).trimmed();

            // Preprocess RHS: substitute multi-word display names, then apply percent patterns.
            QString rhs = applyPercentPreprocess(substituteVars(rawRhs));

            // Try currency evaluation on the RHS before libqalculate.
            // tryEvaluateCurrencyExpr looks up m_varCurrencyTag by display name — pass original RHS.
            QString rhsForCurrency = applyPercentPreprocess(rawRhs);
            {
                QString currResult, currTag;
                if (tryEvaluateCurrencyExpr(rhsForCurrency, &currResult, &currTag)) {
                    double numVal = 0.0;
                    parseDisplayNumber(currResult, &numVal);
                    if (std::isfinite(numVal)) {
                        m_varCurrencyTag[displayName]  = currTag;
                        m_varNumericValue[displayName] = numVal;
                        m_userVarDisplayValues[displayName] = currResult;
                        // Store as plain number in libqalculate so arithmetic with
                        // other variables continues to work.
                        const QString qa = QStringLiteral("%1 := %2")
                                            .arg(internalName)
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
                    // Set the variable in the calculator using the internal (underscore) name.
                    QString assignment = QString("%1 := %2").arg(internalName, rhs);
                    MathStructure _assignTmp;
                    m_calc->calculate(&_assignTmp, assignment.toStdString(), 5000, eo);

                    if (rhsResult.isNumber()) {
                        double v = rhsResult.number().floatValue();
                        if (!std::isfinite(v)) {
                            res.ok = false;
                            res.error = "Error";
                        } else {
                            m_varNumericValue[displayName] = v;
                            res.result = smartFormat(v, m_decimalPlaces);
                            m_userVarDisplayValues[displayName] = res.result;
                            res.hasNumericValue = true;
                            res.numericValue = v;
                            res.totalKey = totalKey;
                            res.ok = true;
                        }
                    } else {
                        QString rhsStr = QString::fromStdString(m_calc->print(rhsResult, 2000, po_display));
                        res.result = rhsStr;
                        res.ok = !hasCalculationError(m_calc);
                        if (!res.ok) {
                            res.error = "Error";
                        } else {
                            if (res.ok)
                                m_userVarDisplayValues[displayName] = rhsStr;
                            // Re-assign as plain number so downstream arithmetic works.
                            double numericForVar = 0.0;
                            if (parseDisplayNumber(rhsStr, &numericForVar) && std::isfinite(numericForVar)) {
                                m_varNumericValue[displayName] = numericForVar;
                                QString numAssignment = QString("%1 := %2").arg(internalName).arg(numericForVar, 0, 'g', 15);
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

            line = applyPercentPreprocess(substituteVars(line));
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
                    QString out = QString::fromStdString(m_calc->print(resVal, 2000, po_display));
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

    {
        QMutexLocker snapshotLock(&m_snapshotMutex);
        m_snapshotVarNames = m_displayToInternal.keys();
        m_snapshotVarValues.clear();
        for (auto it = m_userVarDisplayValues.constBegin(); it != m_userVarDisplayValues.constEnd(); ++it)
            m_snapshotVarValues.insert(it.key(), it.value());
    }

    return results;
}

// Case-insensitive prefix match against a libqalculate name (std::string).
static bool caseInsensitivePrefixMatch(const std::string &name, const std::string &lowerPrefix)
{
    if (name.size() < lowerPrefix.size()) return false;
    for (size_t i = 0; i < lowerPrefix.size(); ++i)
        if (std::tolower((unsigned char)name[i]) != (unsigned char)lowerPrefix[i]) return false;
    return true;
}

QString QalcBridge::getCompletion(const QString &prefix) {
    if (prefix.isEmpty()) return prefix;
    QMutexLocker locker(&m_calcMutex);

    QStringList matches;
    std::string p = prefix.toLower().toStdString();

    for (size_t i = 0; ; ++i) {
        Unit *u = m_calc->getUnit(i);
        if (!u) break;
        for (size_t j = 0; j < u->countNames(); ++j) {
            const std::string &name = u->getName(j).name;
            if (caseInsensitivePrefixMatch(name, p)) matches << QString::fromStdString(name);
        }
    }
    for (size_t i = 0; ; ++i) {
        Variable *v = m_calc->getVariable(i);
        if (!v) break;
        for (size_t j = 0; j < v->countNames(); ++j) {
            const std::string &name = v->getName(j).name;
            if (caseInsensitivePrefixMatch(name, p)) matches << QString::fromStdString(name);
        }
    }
    for (size_t i = 0; ; ++i) {
        MathFunction *f = m_calc->getFunction(i);
        if (!f) break;
        for (size_t j = 0; j < f->countNames(); ++j) {
            const std::string &name = f->getName(j).name;
            if (caseInsensitivePrefixMatch(name, p)) matches << QString::fromStdString(name);
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

QStringList QalcBridge::getCompletions(const QString &lineContext) {
    if (lineContext.trimmed().isEmpty()) return {};

    // Extract the "current segment" = text after the last operator/separator on this line.
    static const QString separators = QStringLiteral("+-*/()=^%,;");
    int sepPos = -1;
    for (int i = lineContext.length() - 1; i >= 0; --i) {
        if (separators.contains(lineContext[i])) { sepPos = i; break; }
    }
    QString segment = lineContext.mid(sepPos + 1);
    int lead = 0;
    while (lead < segment.length() && segment[lead] == ' ') ++lead;
    segment = segment.mid(lead);

    // For keyword matching: last space-separated token ("30 da" → "da").
    const int lastSpace = segment.lastIndexOf(' ');
    const QString lastWord = lastSpace != -1 ? segment.mid(lastSpace + 1) : segment;

    // For user-variable matching: full segment trimmed ("Var name " → "Var name")
    // so that typing a complete first word + space can complete multi-word variables.
    const QString segmentTrimmed = segment.trimmed();

    if (segmentTrimmed.isEmpty()) return {};

    QStringList result;
    QSet<QString> seen;

    // User-defined variables: match against full segment (supports multi-word completions).
    // Read from the snapshot published by the last evaluateDocument() run, so the
    // GUI thread never touches state the worker thread may be rewriting.
    {
        QMutexLocker snapshotLock(&m_snapshotMutex);
        for (const QString &displayName : std::as_const(m_snapshotVarNames)) {
            if (displayName.startsWith(segmentTrimmed, Qt::CaseInsensitive) && !seen.contains(displayName)) {
                seen.insert(displayName);
                result << (displayName + '\t' + m_snapshotVarValues.value(displayName));
            }
        }
    }

    // Keywords: match against last word only (single-token, never multi-word).
    if (!lastWord.isEmpty()) {
        static const QStringList keywords = {
            "today", "tomorrow", "yesterday", "now",
            "days", "weeks", "months", "years",
            "seconds", "minutes", "hours"
        };
        for (const auto &kw : keywords) {
            if (kw.startsWith(lastWord, Qt::CaseInsensitive) && !seen.contains(kw)) {
                seen.insert(kw);
                result << (kw + '\t');
            }
        }
    }

    std::sort(result.begin(), result.end(), [](const QString &a, const QString &b) {
        return a.section('\t', 0, 0).compare(b.section('\t', 0, 0), Qt::CaseInsensitive) < 0;
    });

    return result;
}

void QalcBridge::loadRatesFromCache()
{
    const QString filePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                             + QStringLiteral("/rates.json");
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isObject())
        return;

    const QJsonObject root = doc.object();
    const QJsonObject fiatObj   = root.value(QStringLiteral("fiat")).toObject();
    const QJsonObject cryptoObj = root.value(QStringLiteral("crypto")).toObject();

    if (!fiatObj.isEmpty()) {
        applyFiatRates(fiatObj);
        m_fiatStatus = NetworkStatus::Success;
    }
    if (!cryptoObj.isEmpty()) {
        applyCryptoRates(cryptoObj);
        m_cryptoStatus = NetworkStatus::Success;
    }
    if (!fiatObj.isEmpty() || !cryptoObj.isEmpty()) {
        emit networkStatusChanged();
        emit ratesUpdated();
    }
}

void QalcBridge::saveRatesToCache(bool isCrypto, const QJsonObject &rates)
{
    const QString cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(cachePath);
    const QString filePath = cachePath + QStringLiteral("/rates.json");

    QJsonObject root;
    {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly)) {
            const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            if (!doc.isNull() && doc.isObject())
                root = doc.object();
        }
    }

    root[QStringLiteral("timestamp")] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    if (isCrypto)
        root[QStringLiteral("crypto")] = rates;
    else
        root[QStringLiteral("fiat")] = rates;

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly))
        file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
}
