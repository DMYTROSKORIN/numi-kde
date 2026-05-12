#include "../src/qalcbridge.h"
#include "../src/documentmodel.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QDate>
#include <QLocale>
#include <QString>
#include <QList>
#include <QTemporaryDir>
#include <QQuickView>
#include <QQmlContext>
#include <QQuickItem>
#include <cstdio>

// Minimal mock for EditorPane QML tests — only Q_INVOKABLEs accessed by EditorPane.qml
class MockDocumentModel : public QObject {
    Q_OBJECT
public:
    explicit MockDocumentModel(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE QStringList getCompletions(const QString &prefix) {
        m_lastQuery = prefix;
        if (prefix.compare("kil", Qt::CaseInsensitive) == 0)
            return {"kilogram", "kilometer", "kilowatt"};
        if (prefix.compare("km", Qt::CaseInsensitive) == 0)
            return {"km"};
        return {};
    }
    Q_INVOKABLE QString completeWord(const QString &p) { return p; }
    Q_INVOKABLE QString highlightExample(const QString &t) { return t; }
    Q_INVOKABLE void saveSession() {}

    QString lastQuery() const { return m_lastQuery; }

private:
    QString m_lastQuery;
};

struct TestCase {
    const char *name;
    const char *input;
    const char *expected; // nullptr = expect ok==true, any value = exact match
    bool expectOk;
};

static int passed = 0;
static int failed = 0;

static void check(const char *name, bool condition, const QString &got, const char *want) {
    if (condition) {
        std::printf("  PASS  %s\n", name);
        ++passed;
    } else {
        std::printf("  FAIL  %s\n        got:      \"%s\"\n        expected: \"%s\"\n",
                    name, got.toUtf8().constData(), want ? want : "(ok=true)");
        ++failed;
    }
}

static void runSuite(QalcBridge &bridge) {
    QJsonObject fiatRates;
    fiatRates.insert("USD", 1.0);
    fiatRates.insert("AED", 0.25);
    fiatRates.insert("EUR", 1.25);
    fiatRates.insert("UAH", 0.025);
    bridge.applyFiatRates(fiatRates);

    auto eval = [&](const QString &expr) -> LineResult {
        auto results = bridge.evaluateDocument(expr);
        return results.isEmpty() ? LineResult{} : results.first();
    };

    // ── Basic arithmetic ─────────────────────────────────────────────────────
    {
        auto r = eval("2 + 2");
        check("2 + 2 = 4", r.ok && r.result == "4", r.result, "4");
    }
    {
        auto r = eval("10 - 3");
        check("10 - 3 = 7", r.ok && r.result == "7", r.result, "7");
    }
    {
        auto r = eval("6 * 7");
        check("6 * 7 = 42", r.ok && r.result == "42", r.result, "42");
    }
    {
        auto r = eval("100 / 4");
        check("100 / 4 = 25", r.ok && r.result == "25", r.result, "25");
    }
    {
        auto r = eval("2 ^ 10");
        check("2 ^ 10 = 1,024", r.ok && r.result == QLocale().toString(1024), r.result, "1,024");
    }

    // ── No trailing zeros on integers ─────────────────────────────────────────
    {
        auto r = eval("500 + 0");
        check("500 no trailing zeros", r.ok && r.result == "500", r.result, "500");
    }
    {
        auto r = eval("1000 / 2");
        check("1000/2 = 500 no decimals", r.ok && r.result == "500", r.result, "500");
    }

    // ── Decimal results ───────────────────────────────────────────────────────
    bridge.setDecimalPlaces(3);
    {
        auto r = eval("1 / 3");
        check("1/3 has decimals", r.ok && r.result.contains('.'), r.result, "0.333");
    }
    {
        auto r = eval("22 / 7");
        check("22/7 ≈ 3.143", r.ok && r.result.startsWith("3."), r.result, "3.143...");
    }

    // ── Percentage ────────────────────────────────────────────────────────────
    {
        auto r = eval("20% from 1000");
        check("20% from 1000 = 200", r.ok && r.result == "200", r.result, "200");
    }
    {
        auto r = eval("20% of 1000");
        check("20% of 1000 = 200", r.ok && r.result == "200", r.result, "200");
    }
    {
        auto r = eval("50% from 200");
        check("50% from 200 = 100", r.ok && r.result == "100", r.result, "100");
    }
    {
        auto r = eval("10% of 50");
        check("10% of 50 = 5", r.ok && r.result == "5", r.result, "5");
    }
    {
        auto r = eval("20% from");
        check("incomplete percent waits for RHS", r.ok && r.result.isEmpty(), r.result, "");
    }
    {
        auto r = eval("600 USD to");
        check("incomplete conversion waits for target", r.ok && r.result.isEmpty(), r.result, "");
    }
    {
        auto r = eval("500/");
        check("incomplete division waits for RHS", r.ok && r.result.isEmpty(), r.result, "");
    }
    {
        auto r = eval("500/0");
        check("division by zero is Error", !r.ok && r.error == "Error", r.error, "Error");
    }

    // ── Variables ────────────────────────────────────────────────────────────
    {
        auto results = bridge.evaluateDocument("x = 42\nx * 2");
        bool ok = results.size() == 2
               && results[0].ok && results[0].result == "42"
               && results[1].ok && results[1].result == "84";
        check("variable assignment and use", ok,
              results.size() >= 2 ? results[1].result : "size<2", "84");
    }
    {
        auto results = bridge.evaluateDocument("price = 100\ntax = 20% from price\ntax");
        bool ok = results.size() == 3
               && results[2].ok && results[2].result == "20";
        check("variable chain: 20% of price=100", ok,
              results.size() >= 3 ? results[2].result : "size<3", "20");
    }
    {
        bridge.evaluateDocument("x = 10");
        auto results = bridge.evaluateDocument("x + 5");
        bool ok = results.size() == 1 && (!results[0].ok || results[0].result != "15");
        check("variable deletion: x should not persist across evaluations", ok,
              results.size() >= 1 ? results[0].result : "no results", "Error/not 15");
    }
    {
        // A = 500 AED to USD → currency-valued result; A+200 must compute the sum,
        // not show an unsimplified expression like "USD 136.xxx + 200.000".
        auto results = bridge.evaluateDocument("A = 500 AED to USD\nA + 200");
        bool line1ok = results.size() >= 1 && results[0].ok && !results[0].result.isEmpty();
        bool line2ok = results.size() >= 2 && results[1].ok
                    && results[1].result != "Error"
                    && !results[1].result.contains("+")
                    && results[1].hasNumericValue
                    && results[1].numericValue > 200.0;
        check("currency variable + plain number computes sum (bug: USD x + 200.000)",
              line1ok && line2ok,
              results.size() >= 2 ? results[1].result : "size<2",
              "single computed number > 200");
    }

    // ── Currency variable arithmetic (m_varCurrencyTag) ──────────────────────
    {
        // A = 500 AED to USD → A has tag USD (125); A + 200 USD must be "USD 325"
        auto r = bridge.evaluateDocument("A = 500 AED to USD\nA + 200 USD");
        bool ok = r.size() == 2
               && r[1].ok
               && !r[1].result.contains('+')
               && r[1].result.startsWith("USD ");
        check("VAR + N CURR: A + 200 USD gives single USD result",
              ok, r.size() >= 2 ? r[1].result : "size<2", "USD 325");
    }
    {
        // A + USD 200 (prefix style currency)
        auto r = bridge.evaluateDocument("A = 500 AED to USD\nA + USD 200");
        bool ok = r.size() == 2
               && r[1].ok
               && !r[1].result.contains('+')
               && r[1].result.startsWith("USD ");
        check("VAR + CURR N: A + USD 200 gives single USD result",
              ok, r.size() >= 2 ? r[1].result : "size<2", "USD 325");
    }
    {
        // A = 500 AED to USD (125 USD); A + 200 USD = USD 325; result carries tag
        auto r = bridge.evaluateDocument("A = 500 AED to USD\nA + 200 USD");
        bool ok = r.size() == 2 && r[1].ok && r[1].result == "USD 325";
        check("VAR + N CURR: exact result is USD 325",
              ok, r.size() >= 2 ? r[1].result : "size<2", "USD 325");
    }
    {
        // B + C where both have same tag (EUR)
        // B = 500 AED to EUR = 500 * 0.25 / 1.25 = 100 EUR
        // C = 300 AED to EUR = 300 * 0.25 / 1.25 = 60 EUR
        // B + C = 160 EUR
        auto r = bridge.evaluateDocument("B = 500 AED to EUR\nC = 300 AED to EUR\nB + C");
        bool ok = r.size() == 3
               && r[2].ok
               && !r[2].result.contains('+')
               && r[2].result.startsWith("EUR ");
        check("VAR1 + VAR2 same currency gives single result",
              ok, r.size() >= 3 ? r[2].result : "size<3", "EUR 160");
    }
    {
        // Assignment with conversion in RHS: A = 500 AED to EUR
        // 500 AED * 0.25 USD/AED / 1.25 USD/EUR = 100 EUR
        auto r = bridge.evaluateDocument("A = 500 AED to EUR");
        bool ok = r.size() == 1 && r[0].ok && r[0].result == "EUR 100";
        check("assignment RHS conversion: A = 500 AED to EUR gives EUR 100",
              ok, r.size() >= 1 ? r[0].result : "size<1", "EUR 100");
    }
    {
        // Multi-currency sum in assignment RHS: A = 500 AED + 400 AED (same currency)
        // 500 * 0.25 + 400 * 0.25 = 225 USD
        // (both are AED, output in AED: 500 + 400 = 900 AED)
        auto r = bridge.evaluateDocument("A = 500 AED + 400 AED");
        bool ok = r.size() == 1 && r[0].ok && r[0].result == "AED 900";
        check("assignment RHS same-currency sum: A = 500 AED + 400 AED = AED 900",
              ok, r.size() >= 1 ? r[0].result : "size<1", "AED 900");
    }
    {
        // Integers must never show trailing .000 in results.
        auto results = bridge.evaluateDocument("200 + 0");
        bool ok = results.size() == 1 && results[0].ok && results[0].result == "200";
        check("integer result has no trailing zeros (200+0 = \"200\")", ok,
              results.size() >= 1 ? results[0].result : "size<1", "200");
    }

    // ── Empty and comment lines ───────────────────────────────────────────────
    {
        auto results = bridge.evaluateDocument("// comment\n1 + 1");
        bool ok = results.size() == 2
               && results[0].ok && results[0].result.isEmpty()
               && results[1].ok && results[1].result == "2";
        check("comment line returns empty result", ok,
              results.size() >= 2 ? results[1].result : "size<2", "2");
    }
    {
        auto results = bridge.evaluateDocument("# hash comment\n3 + 3");
        bool ok = results.size() == 2
               && results[0].ok && results[0].result.isEmpty()
               && results[1].ok && results[1].result == "6";
        check("# comment line returns empty result", ok,
              results.size() >= 2 ? results[1].result : "size<2", "6");
    }
    {
        auto results = bridge.evaluateDocument("\n2 + 2");
        bool ok = results.size() == 2
               && results[0].ok && results[0].result.isEmpty()
               && results[1].ok && results[1].result == "4";
        check("empty line returns empty result", ok,
              results.size() >= 2 ? results[1].result : "size<2", "4");
    }

    // ── /help command ─────────────────────────────────────────────────────────
    {
        auto results = bridge.evaluateDocument("/help");
        // /help is handled at DocumentModel level; qalcbridge just evaluates it.
        // It will either fail or return a non-critical result — don't assert on value.
        check("/help line does not crash", results.size() >= 1, "ok", "no crash");
    }

    // ── Date/time keywords ───────────────────────────────────────────────────
    {
        auto r = eval("time");
        check("time returns current time", r.ok && !r.result.isEmpty(),
              r.result, "HH:mm:ss");
    }
    {
        auto r = eval("now");
        check("now returns current date and time", r.ok && r.result.contains(' '),
              r.result, "yyyy-MM-dd HH:mm:ss");
    }
    {
        const QString date = QDate::currentDate().addYears(-2).addDays(-5).toString("dd.MM.yyyy");
        auto r = eval(QStringLiteral("today - %1").arg(date));
        check("today minus date returns detailed English span",
              r.ok && r.result == "2 years and 5 days; 735 days.",
              r.result, "2 years and 5 days; 735 days.");
    }
    {
        auto r = eval("01.01.2000 - 02.05.2026");
        const QString expected = QStringLiteral("26 years, 4 months and 1 day; %1 days.")
            .arg(QLocale(QLocale::English, QLocale::UnitedStates).toString(QDate(2000, 1, 1).daysTo(QDate(2026, 5, 2))));
        check("date minus date returns detailed English span",
              r.ok && r.result == expected,
              r.result, expected.toUtf8().constData());
    }
    {
        auto r = eval("01.02.2012 - today");
        const bool ok = r.ok
                     && !r.result.startsWith('-')
                     && !r.result.contains("excluding")
                     && r.result.contains(';')
                     && r.result.endsWith("days.");
        check("date minus today returns compact absolute span",
              ok, r.result, "compact positive date span");
    }
    {
        auto r = eval("01.01.2000 + 25 years");
        check("date plus years returns date",
              r.ok && r.result == "01.01.2025",
              r.result, "01.01.2025");
    }
    {
        auto r = eval("today - 01.01.2000");
        check("today is highlighted as an operator",
              r.highlightedHtml.contains("#ffd35a") && r.highlightedHtml.contains("today"),
              r.highlightedHtml, "highlighted today");
    }

    // ── Unit conversion ───────────────────────────────────────────────────────
    // Results include the target unit (e.g. "1,000 m"), which is correct behavior.
    {
        auto r = eval("1 km to m");
        check("1 km to m contains 1,000", r.ok && r.result.startsWith(QLocale().toString(1000)), r.result, "1,000 m");
    }
    {
        auto r = eval("1 hour to minutes");
        check("1 hour to minutes contains 60", r.ok && r.result.startsWith("60"), r.result, "60 min");
    }
    {
        auto r = eval("100 cm to m");
        check("100 cm to m contains 1", r.ok && r.result.startsWith("1"), r.result, "1 m");
    }
    {
        auto r = eval("1 kg to g");
        check("1 kg to g contains 1,000", r.ok && r.result.startsWith(QLocale().toString(1000)), r.result, "1,000 g");
    }

    // ── Currency preprocessing (lowercase) ───────────────────────────────────
    {
        auto r = eval("100 usd to USD");
        check("lowercase 'usd' preprocessed", r.ok, r.result, "(non-empty)");
    }
    {
        auto r = eval("500 AED to USD");
        check("Frankfurter fiat conversion uses injected USD rates",
              r.ok && r.result == "USD 125" && r.hasNumericValue && r.numericValue == 125.0 && r.totalKey == "USD",
              QString("%1 (%2)").arg(r.result, QString::number(r.numericValue)), "USD 125");
    }

    // ── Cryptocurrency conversion ───────────────────────────────────────────
    {
        QJsonObject rates;
        rates.insert("BTC", 50000.0);
        rates.insert("ETH", 2500.0);
        bridge.applyCryptoRates(rates);

        auto btcToEth = eval("1 BTC to ETH");
        check("1 BTC to ETH = ETH 20", btcToEth.ok && btcToEth.result == "ETH 20",
              btcToEth.result, "ETH 20");

        auto usdToEth = eval("400 USD to ETH");
        check("400 USD to ETH = ETH 0.16", usdToEth.ok && usdToEth.result == "ETH 0.16",
              usdToEth.result, "ETH 0.16");

        auto ethToUsd = eval("1 ETH to USD");
        check("1 ETH to USD = USD 2,500", ethToUsd.ok && ethToUsd.result == QStringLiteral("USD %1").arg(QLocale().toString(2500)),
              ethToUsd.result, "USD 2,500");
        check("crypto conversion exposes numeric value",
              ethToUsd.hasNumericValue && ethToUsd.numericValue == 2500.0,
              QString::number(ethToUsd.numericValue), "2500");

        auto btcToEur = eval("1 BTC to EUR");
        check("1 BTC to EUR uses Frankfurter fiat target rate",
              btcToEur.ok && btcToEur.result == QStringLiteral("EUR %1").arg(QLocale().toString(40000)) && btcToEur.hasNumericValue && btcToEur.numericValue == 40000.0,
              btcToEur.result, "EUR 40,000");

        auto btcToUah = eval("1 BTC to UAH");
        check("1 BTC to UAH supports fiat target",
              btcToUah.ok && btcToUah.result.startsWith("UAH ") && btcToUah.hasNumericValue && btcToUah.numericValue > 0.0 && btcToUah.totalKey == "UAH",
              QString("%1 (%2)").arg(btcToUah.result, QString::number(btcToUah.numericValue)),
              "UAH <number>");

        auto mixed = bridge.evaluateDocument("500 AED to USD\n1 BTC to UAH");
        bool ok = mixed.size() == 2
               && mixed[0].ok && mixed[0].hasNumericValue && mixed[0].numericValue > 0.0
               && mixed[1].ok && mixed[1].hasNumericValue && mixed[1].numericValue > 0.0;
        check("mixed fiat and crypto conversion rows expose numeric values",
              ok,
              mixed.size() >= 2
                ? QString("%1 + %2").arg(QString::number(mixed[0].numericValue),
                                         QString::number(mixed[1].numericValue))
                : "size<2",
              "two numeric conversion rows");
    }

    // ── Math functions ────────────────────────────────────────────────────────
    {
        auto r = eval("sqrt(9)");
        check("sqrt(9) = 3", r.ok && r.result == "3", r.result, "3");
    }
    {
        auto r = eval("sqrt(2)");
        check("sqrt(2) has decimals", r.ok && r.result.startsWith("1."), r.result, "1.414...");
    }
    {
        auto r = eval("abs(-5)");
        check("abs(-5) = 5", r.ok && r.result == "5", r.result, "5");
    }

    // ── Total across multiple lines ───────────────────────────────────────────
    {
        auto results = bridge.evaluateDocument("100\n200\n300");
        bool ok = results.size() == 3
               && results[0].result == "100"
               && results[1].result == "200"
               && results[2].result == "300";
        check("multi-line numeric results", ok,
              results.size() >= 3 ? results[2].result : "size<3", "300");
    }

    // ── Large numbers ─────────────────────────────────────────────────────────
    {
        auto r = eval("1000000 + 1");
        check("1,000,000 + 1 = 1,000,001", r.ok && r.result == QLocale().toString(1000001), r.result, "1,000,001");
    }
    {
        auto r = eval("999 * 999");
        check("999 * 999 = 998,001", r.ok && r.result == QLocale().toString(998001), r.result, "998,001");
    }

    // ── Negative numbers ─────────────────────────────────────────────────────
    {
        auto r = eval("-5 + 3");
        check("-5 + 3 = -2", r.ok && r.result == "-2", r.result, "-2");
    }
    {
        auto r = eval("0 - 7");
        check("0 - 7 = -7", r.ok && r.result == "-7", r.result, "-7");
    }

    // ── Completion ────────────────────────────────────────────────────────────
    {
        QString c = bridge.getCompletion("kil");
        check("completion 'kil' starts with 'kil'",
              c.toLower().startsWith("kil"), c, "kil...");
    }
    {
        auto results = bridge.evaluateDocument("m = 10\n5 m");
        // We now ALLOW shadowing units (like 'm' for meter).
        // m = 10, then 5 m = 50.
        bool ok = results.size() == 2 && results[0].ok && results[1].ok && results[1].result == "50";
        check("assignment to unit name 'm' is now allowed (shadowing)", ok,
              results.size() >= 2 ? results[1].result : "no results", "50");
    }
    {
        auto results = bridge.evaluateDocument("2 : 4");
        check("colon division support (2 : 4 = 0.5)",
              results.size() == 1 && results[0].result == "0.5", results.size() >= 1 ? results[0].result : "error", "0.5");
    }
    {
        auto results = bridge.evaluateDocument("asdasdasd");
        QString actual = results.isEmpty() ? "empty list" : (results[0].ok ? results[0].result : "Error: " + results[0].error);
        if (actual.isEmpty()) actual = "empty";
        check("ignore pure text junk",
              results.size() == 1 && results[0].result.isEmpty() && results[0].ok, actual, "empty");
    }
    {
        auto results = bridge.evaluateDocument("500.42 , 599 58");
        check("reject vector/list input with Error",
              results.size() == 1 && !results[0].ok && results[0].error == "Error", "result shown", "Error");
    }
    {
        auto results = bridge.evaluateDocument("/helpjunk");
        check("ignore invalid /help commands",
              results.size() == 1 && results[0].result.isEmpty() && results[0].ok, "junk shown", "empty");
    }
    {
        auto results = bridge.evaluateDocument("7,486,332.414 + 1");
        check("robust thousands separator removal",
              results.size() == 1 && results[0].result == QLocale().toString(7486333.414, 'f', 3), 
              results.size() >= 1 ? results[0].result : "error", "7,486,333.414");
    }
    {
        auto results = bridge.evaluateDocument("UAH 22045 - 600");
        const QString expected = QStringLiteral("UAH %1").arg(QLocale().toString(21445));
        check("Numi-style unit propagation (UAH 22045 - 600)",
              results.size() == 1 && results[0].result == expected,
              results.size() >= 1 ? results[0].result : "error", expected.toUtf8().constData());
    }
    {
        // Mixed currencies now output in the default currency (USD by default).
        // 500 EUR * 1.25 USD/EUR - 100 USD = 625 - 100 = 525 USD
        auto results = bridge.evaluateDocument("500 EUR - 100 USD");
        check("cross-currency uses default currency as output",
              results.size() == 1 && results[0].result == "USD 525",
              results.size() >= 1 ? results[0].result : "error", "USD 525");
    }
    {
        // Explicit "to CURR" still overrides default currency.
        // 500 EUR - 100 USD → EUR: 500 - 100/1.25 = 500 - 80 = 420 EUR
        auto results = bridge.evaluateDocument("500 EUR - 100 USD to EUR");
        check("explicit to EUR overrides default currency in mixed expression",
              results.size() == 1 && results[0].result == "EUR 420",
              results.size() >= 1 ? results[0].result : "error", "EUR 420");
    }

    // ── getCompletions ────────────────────────────────────────────────────────
    // getCompletions(lineContext) returns user-defined variables and keywords
    // matching the current word (text after the last operator on the line).
    // Format: "name\tvalue" — tab separates name from last computed value.
    {
        // Keyword "today" matches prefix "tod"
        auto list = bridge.getCompletions("tod");
        check("getCompletions 'tod' matches keyword 'today'",
              !list.isEmpty() && list[0].section('\t', 0, 0) == "today",
              list.isEmpty() ? "empty" : list[0].section('\t', 0, 0), "today");
    }
    {
        // "to" matches both "today" and "tomorrow" — at least 2 results
        auto list = bridge.getCompletions("to");
        check("getCompletions 'to' returns multiple keyword results",
              list.size() >= 2, QString::number(list.size()), ">=2");
    }
    {
        // All names returned must actually start with the queried prefix
        auto list = bridge.getCompletions("to");
        bool allMatch = !list.isEmpty() && std::all_of(list.cbegin(), list.cend(),
            [](const QString &s) { return s.section('\t', 0, 0).toLower().startsWith("to"); });
        check("getCompletions results all start with the query prefix",
              allMatch, allMatch ? QString("ok (%1 items)").arg(list.size()) : list.join(", "),
              "all start with 'to'");
    }
    {
        // User-defined variable appears in completions after evaluateDocument
        bridge.evaluateDocument("myvar = 42");
        auto list = bridge.getCompletions("myv");
        check("getCompletions finds user-defined variable 'myvar'",
              !list.isEmpty() && list[0].section('\t', 0, 0) == "myvar",
              list.isEmpty() ? "empty" : list[0].section('\t', 0, 0), "myvar");
    }
    {
        // Result includes value after tab separator
        auto list = bridge.getCompletions("myv");
        check("getCompletions result includes value after tab",
              !list.isEmpty() && list[0].contains('\t'),
              list.isEmpty() ? "no results" : list[0], "myvar\\t<value>");
    }
    {
        // Case-insensitive prefix match for user variable
        auto list = bridge.getCompletions("MYV");
        check("getCompletions case-insensitive match for user variable",
              !list.isEmpty() && list[0].section('\t', 0, 0).compare("myvar", Qt::CaseInsensitive) == 0,
              list.isEmpty() ? "empty" : list[0].section('\t', 0, 0), "myvar");
    }
    {
        // Prefix extracted from line context: text after the last separator
        auto list = bridge.getCompletions("100 + myv");
        check("getCompletions extracts prefix after '+' separator",
              !list.isEmpty() && list[0].section('\t', 0, 0) == "myvar",
              list.isEmpty() ? "empty" : list[0].section('\t', 0, 0), "myvar");
    }
    {
        auto list = bridge.getCompletions("xyznoexist");
        check("getCompletions no match returns empty list",
              list.isEmpty(), QString::number(list.size()), "0");
    }
    {
        auto list = bridge.getCompletions("");
        check("getCompletions empty prefix returns empty list",
              list.isEmpty(), QString::number(list.size()), "0");
    }
    {
        // Results sorted alphabetically by name
        auto list = bridge.getCompletions("to");
        bool sorted = true;
        for (int i = 1; i < list.size(); ++i) {
            if (list[i].section('\t', 0, 0).toLower() < list[i-1].section('\t', 0, 0).toLower())
                { sorted = false; break; }
        }
        check("getCompletions results sorted alphabetically",
              !list.isEmpty() && sorted,
              sorted ? QString("ok (%1 items)").arg(list.size()) : list.join(", "),
              "sorted");
    }
    {
        // Result names must not contain slashes or carets
        auto list = bridge.getCompletions("to");
        bool clean = true;
        for (const auto &item : list) {
            QString name = item.section('\t', 0, 0);
            if (name.contains('/') || name.contains('^')) { clean = false; break; }
        }
        check("getCompletions result names contain no slashes or carets",
              !list.isEmpty() && clean,
              clean ? QString("ok (%1 items)").arg(list.size()) : "bad item found",
              "no / ^");
    }
}

#include <QSignalSpy>
#include <QTest>

static void runDocumentModelSuite() {
    DocumentModel model;
    QSignalSpy spy(&model, &DocumentModel::linesChanged);

    auto wait = [&]() {
        if (!spy.isEmpty()) spy.clear();
        // Wait for up to 2000ms. Since we have a 50ms debounce + evaluation time, this is plenty.
        spy.wait(2000);
    };

    // ── DocumentModel totals ─────────────────────────────────────────────────
    {
        model.setSource("100\n200\n300");
        wait();
        bool ok = model.rowCount() == 3
               && model.resultCount() == 3
               && model.errorCount() == 0
               && model.hasTotal()
               && model.total() == 600.0;
        check("DocumentModel total = 600", ok,
              QString::number(model.total()), "600");
    }
    {
        model.setSource("1 km to m\n2 km to m");
        wait();
        bool ok = model.rowCount() == 2
               && model.resultCount() == 2
               && model.errorCount() == 0
               && model.hasTotal()
               && model.total() == 3000.0;
        check("DocumentModel total includes converted unit results", ok,
              QString::number(model.total()), "3000");
    }
    {
        model.setSource("500 AED to USD\n100");
        wait();
        bool ok = model.rowCount() == 2
               && model.resultCount() == 2
               && model.errorCount() == 0
               && !model.hasTotal();
        check("DocumentModel total rejects mixed dimensions", ok,
              model.hasTotal() ? QString::number(model.total()) : "no total", "no total");
    }
    {
        model.setSource("500 AED to USD\n100 USD to UAH");
        wait();
        bool ok = model.rowCount() == 2
               && model.resultCount() == 2
               && model.errorCount() == 0
               && !model.hasTotal();
        check("DocumentModel total rejects different currency targets", ok,
              model.hasTotal() ? QString::number(model.total()) : "no total", "no total");
    }
    {
        const QString date = QDate::currentDate().addYears(-2).addDays(-5).toString("dd.MM.yyyy");
        model.setSource(QStringLiteral("today - %1\n100").arg(date));
        wait();
        bool ok = model.rowCount() == 2
               && model.resultCount() == 2
               && model.errorCount() == 0
               && !model.hasTotal();
        check("DocumentModel total ignores date span text", ok,
              model.hasTotal() ? QString::number(model.total()) : "no total", "no total");
    }

    // ── History persistence behavior ────────────────────────────────────────
    {
        model.setSource("1 + 1");
        model.saveSession();
        model.saveSession();

        const auto history = model.history();
        bool ok = history.size() == 1
               && history.first().toMap().value("text").toString() == "1 + 1";
        check("DocumentModel history saves once", ok,
              QString::number(history.size()), "1");

        model.clearHistory();
        check("DocumentModel clearHistory", model.history().isEmpty(),
              QString::number(model.history().size()), "0");
    }

    // ── Autostart desktop file ───────────────────────────────────────────────
    {
        model.setAutostart(true);
        const QString path = QDir::homePath() + "/.config/autostart/numi-kde.desktop";
        bool enabled = model.autostart() && QFile::exists(path);
        check("DocumentModel autostart creates desktop file", enabled,
              QFile::exists(path) ? "exists" : "missing", "exists");

        model.setAutostart(false);
        bool disabled = !model.autostart() && !QFile::exists(path);
        check("DocumentModel autostart removes desktop file", disabled,
              QFile::exists(path) ? "exists" : "missing", "missing");
    }

    // ── /help command ────────────────────────────────────────────────────────
    {
        model.setSource("/help");
        const auto first = model.index(0, 0);
        const QString help = model.data(first, DocumentModel::ResultRole).toString();
        const QString html = model.data(first, DocumentModel::HighlightedHtmlRole).toString();
        bool ok = model.rowCount() == 1 && help.isEmpty() && html.contains("/help");
        check("DocumentModel /help returns empty result but highlighted HTML", ok,
              help, "empty result");
    }
    {
        const QString html = model.highlightExample("500 AED to USD");
        bool ok = html.contains("#6fc4e8") && html.contains("AED") && html.contains("USD")
               && html.contains("#ffd35a") && html.contains("to");
        check("DocumentModel help examples use shared syntax highlighting", ok,
              html, "highlighted AED/USD/to");
    }

    // ── getCompletions via DocumentModel ─────────────────────────────────────
    {
        // After evaluating a doc with a variable, completion finds it
        model.setSource("myvar = 42");
        wait();
        QStringList completions = model.getCompletions("myv");
        check("DocumentModel getCompletions finds user-defined variable",
              !completions.isEmpty() && completions[0].startsWith("myvar"),
              completions.isEmpty() ? "empty" : completions[0], "myvar*");
    }
    {
        // Keywords always available regardless of document content
        QStringList completions = model.getCompletions("tod");
        check("DocumentModel getCompletions finds 'today' keyword",
              !completions.isEmpty() && completions[0].startsWith("today"),
              completions.isEmpty() ? "empty" : completions[0], "today*");
    }
    {
        QStringList completions = model.getCompletions("");
        check("DocumentModel getCompletions empty prefix returns empty",
              completions.isEmpty(), QString::number(completions.size()), "0");
    }
}

// ── EditorPane QML key handler tests ─────────────────────────────────────────
// These test that key handlers in EditorPane.qml do not accidentally consume
// events (e.g. the v0.1.48 regression where Enter never created a new line).
static void runEditorSuite() {
    MockDocumentModel mock;

    QQuickView view;
    view.rootContext()->setContextProperty("documentModel", &mock);
    view.setSource(QUrl::fromLocalFile(QStringLiteral(NUMI_KDE_QML_DIR "/EditorPane.qml")));
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.resize(600, 400);
    view.show();
    QTest::qWait(300);

    QQuickItem *root = view.rootObject();
    if (!root || view.status() != QQuickView::Ready) {
        check("EditorPane QML loads successfully", false,
              view.errors().isEmpty() ? "null root" : view.errors().first().toString(),
              "Ready");
        return;
    }
    check("EditorPane QML loads successfully", true, "ok", "ok");

    root->forceActiveFocus();
    QTest::qWait(100);

    auto setText = [&](const QString &t) {
        root->setProperty("text", t);
        QTest::qWait(30);
        QTest::keyClick(&view, Qt::Key_End, Qt::ControlModifier);
        QTest::qWait(20);
    };

    // ── Regression: Enter must create a new line (broke in v0.1.47) ──────────
    {
        setText("2 + 2");
        QTest::keyClick(&view, Qt::Key_Return);
        QTest::qWait(60);
        QString text = root->property("text").toString();
        check("Enter key creates new line (regression: popup handler must not consume Enter)",
              text.contains('\n'),
              QString("got: \"%1\"").arg(text.left(30)), "2 + 2\\n");
    }

    // ── Multiple Enter presses each create a new line ─────────────────────────
    {
        setText("line1");
        QTest::keyClick(&view, Qt::Key_Return);
        QTest::keyClick(&view, Qt::Key_Return);
        QTest::qWait(60);
        int newlines = root->property("text").toString().count('\n');
        check("Multiple Enter presses produce multiple new lines",
              newlines >= 2, QString::number(newlines), ">=2");
    }

    // ── Tab with no completions leaves text unchanged ─────────────────────────
    {
        setText("xyznoexist");
        QString before = root->property("text").toString().trimmed();
        QTest::keyClick(&view, Qt::Key_Tab);
        QTest::qWait(60);
        QString after = root->property("text").toString().trimmed();
        check("Tab with no match leaves text unchanged",
              after == before, after, before.toUtf8().constData());
    }

    // ── Tab queries getCompletions with the current word ─────────────────────
    {
        setText("kil");
        QTest::keyClick(&view, Qt::Key_Tab);
        QTest::qWait(60);
        check("Tab key passes word prefix to getCompletions",
              mock.lastQuery().compare("kil", Qt::CaseInsensitive) == 0,
              mock.lastQuery(), "kil");
    }

    // ── Tab with single match completes without popup ─────────────────────────
    {
        setText("km");
        QTest::keyClick(&view, Qt::Key_Tab);
        QTest::qWait(60);
        check("Tab with single match queries getCompletions",
              mock.lastQuery().compare("km", Qt::CaseInsensitive) == 0,
              mock.lastQuery(), "km");
    }

    // ── Esc key does not crash the editor ─────────────────────────────────────
    {
        setText("hello");
        QTest::keyClick(&view, Qt::Key_Escape);
        QTest::qWait(60);
        check("Esc key does not crash the editor",
              view.rootObject() != nullptr, "ok", "ok");
    }
}

int main(int argc, char *argv[]) {
    QTemporaryDir tempHome;
    if (!tempHome.isValid()) {
        std::printf("Unable to create temporary HOME for tests\n");
        return 1;
    }

    qputenv("HOME", tempHome.path().toUtf8());
    qputenv("XDG_CONFIG_HOME", tempHome.filePath(".config").toUtf8());
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "offscreen");

    QApplication app(argc, argv);
    app.setApplicationName("numi-kde-test");
    app.setOrganizationName("numi-kde");

    const QString suite = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("qalc");

    if (suite == QStringLiteral("documentmodel")) {
        std::printf("=== numi-kde DocumentModel Test Suite ===\n\n");
        runDocumentModelSuite();
    } else if (suite == QStringLiteral("editor")) {
        std::printf("=== numi-kde Editor Key Handler Test Suite ===\n\n");
        runEditorSuite();
    } else {
        std::printf("=== numi-kde QalcBridge Test Suite ===\n\n");
        QalcBridge bridge;
        bridge.setDecimalPlaces(3);
        runSuite(bridge);
    }

    std::printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);
    return failed > 0 ? 1 : 0;
}

#include "qalc_test.moc"
