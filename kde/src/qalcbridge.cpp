#include "qalcbridge.h"
#include "syntaxhighlighter.h"
#include <libqalculate/qalculate.h>
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
    connect(m_nam, &QNetworkAccessManager::finished,
            this, &QalcBridge::onCryptoReply);

    // Fetch on startup, then refresh every hour
    m_cryptoRefreshTimer = new QTimer(this);
    m_cryptoRefreshTimer->setInterval(60 * 60 * 1000);
    connect(m_cryptoRefreshTimer, &QTimer::timeout,
            this, &QalcBridge::fetchCryptoRates);
    m_cryptoRefreshTimer->start();

    fetchCryptoRates();
}

QalcBridge::~QalcBridge() {
    delete m_highlighter;
    delete m_calc;
}

void QalcBridge::fetchCryptoRates() {
    m_networkStatus = NetworkStatus::Fetching;
    emit networkStatusChanged();

    QStringList ids;
    for (const auto &p : CRYPTO_LIST)
        ids << p.first;

    QString url = "https://api.coingecko.com/api/v3/simple/price?ids=" +
                  ids.join(",") +
                  "&vs_currencies=usd";
    m_nam->get(QNetworkRequest(QUrl(url)));
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
        m_networkStatus = NetworkStatus::Success;
        emit networkStatusChanged();
        applyCryptoRates(rates);
        emit cryptoRatesUpdated();
    } else {
        m_networkStatus = NetworkStatus::Error;
        emit networkStatusChanged();
    }
}

void QalcBridge::applyCryptoRates(const QJsonObject &rates) {
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

static bool isCryptoSymbol(const QHash<QString, double> &rates, const QString &symbol)
{
    return rates.contains(symbol.toUpper());
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

QString QalcBridge::convertCryptoExpression(const QString &expression, bool *converted) const
{
    if (converted)
        *converted = false;

    static QRegularExpression cryptoConversionRegex(
        "^\\s*([+-]?(?:\\d+(?:\\.\\d+)?|\\.\\d+))\\s+([A-Za-z]{2,5})\\s+to\\s+([A-Za-z]{2,5})\\s*$",
        QRegularExpression::CaseInsensitiveOption);

    const auto match = cryptoConversionRegex.match(expression);
    if (!match.hasMatch())
        return {};

    const double amount = match.captured(1).toDouble();
    const QString from = match.captured(2).toUpper();
    const QString to = match.captured(3).toUpper();

    auto rateFor = [this](const QString &symbol, bool *ok) -> double {
        if (symbol == QStringLiteral("USD")) {
            *ok = true;
            return 1.0;
        }
        const auto it = m_cryptoUsdRates.constFind(symbol);
        if (it == m_cryptoUsdRates.constEnd()) {
            if (!m_calc->getUnit(symbol.toStdString())) {
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
            MathStructure rateResult = m_calc->calculate(
                QStringLiteral("1 %1 to USD").arg(symbol).toStdString(), eo);
            if (hasCalculationError(m_calc) || rateResult.isUndefined() || rateResult.isInfinite()) {
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
        *ok = true;
        return it.value();
    };

    bool fromOk = false;
    bool toOk = false;
    const bool includesCrypto = isCryptoSymbol(m_cryptoUsdRates, from) || isCryptoSymbol(m_cryptoUsdRates, to);
    if (!includesCrypto)
        return {};

    const double fromRate = rateFor(from, &fromOk);
    const double toRate = rateFor(to, &toOk);
    if (!fromOk || !toOk || toRate <= 0.0)
        return {};

    if (converted)
        *converted = true;
    return QStringLiteral("%1 %2").arg(to, smartFormat(amount * fromRate / toRate, m_decimalPlaces));
}

QString QalcBridge::highlightLine(const QString &line) const
{
    return m_highlighter->highlightLine(line, {});
}

QList<LineResult> QalcBridge::evaluateDocument(const QString &source) {
    QMutexLocker locker(&m_calcMutex);

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

    // First pass: collect variable names for highlighting
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

        bool cryptoConverted = false;
        const QString cryptoResult = convertCryptoExpression(line, &cryptoConverted);
        if (cryptoConverted) {
            res.ok = true;
            res.result = cryptoResult;
            res.hasNumericValue = parseDisplayNumber(cryptoResult, &res.numericValue);
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
        QRegularExpressionMatchIterator currIt = currencyRegex.globalMatch(line);
        // Collect replacements backwards to preserve positions
        struct Replacement { qsizetype start; qsizetype len; QString replacement; };
        QList<Replacement> replacements;
        while (currIt.hasNext()) {
            auto m = currIt.next();
            QString word = m.captured(1);
            QString upper = word.toUpper();
            if (!skipWords.contains(word) && m_calc->getUnit(upper.toStdString())) {
                replacements.prepend({m.capturedStart(1), m.capturedLength(1), upper});
            }
        }
        for (const auto &r : replacements)
            line.replace(r.start, r.len, r.replacement);

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
            
            // Validation: prevent overwriting known units or keywords
            if (m_calc->getUnit(varName.toStdString()) || 
                m_calc->getVariable(varName.toStdString()) ||
                isIncompleteExpression(varName)) {
                res.ok = false;
                res.error = QStringLiteral("Reserved name: %1").arg(varName);
                results.append(res);
                continue;
            }

            QString rhs = applyPercentPreprocess(assignMatch.captured(2));
            try {
                m_calc->clearMessages();
                MathStructure rhsResult = m_calc->calculate(rhs.toStdString(), eo);
                if (hasCalculationError(m_calc) || rhsResult.isUndefined() || rhsResult.isInfinite()) {
                    res.ok = false;
                    res.error = "Error";
                } else if (rhsResult.isNumber()) {
                    double v = rhsResult.number().floatValue();
                    if (!std::isfinite(v)) {
                        res.ok = false;
                        res.error = "Error";
                    } else {
                        QString formatted = smartFormat(v, m_decimalPlaces);
                        m_calc->clearMessages();
                        m_calc->calculate(
                            QString("%1 := %2").arg(varName, formatted).toStdString(), eo);
                        res.result = formatted;
                        res.hasNumericValue = true;
                        res.numericValue = v;
                        res.totalKey = totalKey;
                        res.ok = !hasCalculationError(m_calc);
                        if (!res.ok)
                            res.error = "Error";
                    }
                } else {
                    QString rhsStr = QString::fromStdString(m_calc->print(rhsResult, 2000, po));
                    m_calc->clearMessages();
                    m_calc->calculate(
                        QString("%1 := %2").arg(varName, rhsStr).toStdString(), eo);
                    res.result = rhsStr;
                    res.ok = !hasCalculationError(m_calc);
                    if (!res.ok)
                        res.error = "Error";
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
                MathStructure result = m_calc->calculate(line.toStdString(), eo);
                if (hasCalculationError(m_calc) || result.isUndefined() || result.isInfinite()) {
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
                    QString out = QString::fromStdString(m_calc->print(result, 2000, po));
                    // Strip surrounding quotes (dates, strings)
                    if (out.startsWith('"') && out.endsWith('"'))
                        out = out.mid(1, out.length() - 2);
                    res.result = out;
                    static QRegularExpression conversionWord(
                        "\\b(?:to|in|as)\\b",
                        QRegularExpression::CaseInsensitiveOption);
                    if (conversionWord.match(trimmed).hasMatch())
                        res.hasNumericValue = parseDisplayNumber(out, &res.numericValue);
                    if (res.hasNumericValue)
                        res.totalKey = totalKey;
                    res.ok = true;
                }
            } catch (...) {
                res.ok = false;
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
