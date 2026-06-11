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
            return {"kilogram\t", "kilometer\t", "kilowatt\t"};
        if (prefix.compare("km", Qt::CaseInsensitive) == 0)
            return {"km\t"};
        // Used to test last-word extraction: lineContext "200 - 30 da" ends with "da"
        if (prefix.endsWith("da", Qt::CaseInsensitive))
            return {"days\t"};
        // Multi-word variable test: segment "Var name" (with or without trailing space)
        if (prefix.trimmed().compare("Var name", Qt::CaseInsensitive) == 0)
            return {"Var name extended\t1000"};
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
    bridge.setDecimalPlaces(0); // baseline: integers shown without decimal point
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

    // ── decimal places = 0: integers have no decimal point ────────────────────
    {
        auto r = eval("500 + 0");
        check("places=0: 500 shows without decimal point", r.ok && r.result == "500", r.result, "500");
    }
    {
        auto r = eval("1000 / 2");
        check("places=0: 1000/2 = 500 shows without decimal point", r.ok && r.result == "500", r.result, "500");
    }

    // ── Decimal places display (configurable precision) ───────────────────────
    bridge.setDecimalPlaces(2);
    {
        auto r = eval("10000 + 1");
        check("places=2: integer 10001 shows without trailing zeros",
              r.ok && r.result == QLocale().toString(10001), r.result, "10,001");
    }
    {
        auto r = eval("500 + 0");
        check("places=2: integer 500 shows without trailing zeros",
              r.ok && r.result == "500", r.result, "500");
    }
    {
        auto r = eval("1 / 3");
        check("places=2: 1/3 shows as 0.33",
              r.ok && r.result == QLocale().toString(1.0/3.0, 'f', 2), r.result, "0.33");
    }
    bridge.setDecimalPlaces(0); // reset

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
    bridge.setDecimalPlaces(0); // reset for subsequent integer-result sections

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
        auto r = eval("10,5% of 200");
        check("10,5% of 200 = 21 (comma decimal)", r.ok && r.result == "21", r.result, "21");
    }
    {
        auto r = eval("50,0% from 200");
        check("50,0% from 200 = 100 (comma decimal)", r.ok && r.result == "100", r.result, "100");
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
    // ── Variable name collision detection ─────────────────────────────────────
    {
        // "monthly income" and "monthly_income" both normalise to "monthly_income".
        // Both assignment lines must produce an error, not silently overwrite each other.
        auto results = bridge.evaluateDocument("monthly income = 5\nmonthly_income = 10");
        bool bothError = results.size() == 2 && !results[0].ok && !results[1].ok
                      && results[0].error.contains("conflict")
                      && results[1].error.contains("conflict");
        check("variable name collision: both lines get error",
              bothError,
              results.size() == 2
                  ? QString("line0.ok=%1 line1.ok=%2").arg(results[0].ok).arg(results[1].ok)
                  : "wrong size",
              "line0.ok=false line1.ok=false");
    }
    {
        // No collision: names that differ after normalisation work fine.
        auto results = bridge.evaluateDocument("monthly income = 5\nmonthly expenses = 10");
        bool ok = results.size() == 2 && results[0].ok && results[1].ok;
        check("variable names without collision both succeed",
              ok, QString("line0=%1 line1=%2").arg(results[0].ok).arg(results[1].ok),
              "line0=true line1=true");
    }
    {
        // Triple-space and single-underscore: all three normalise to same internalName.
        auto results = bridge.evaluateDocument("a b = 1\na_b = 2\na  b = 3");
        bool allError = results.size() == 3
                     && !results[0].ok && !results[1].ok && !results[2].ok;
        // results[0] is first — it was added to map, but still flagged because
        // results[1] creates a collision detected during pre-scan.
        check("triple collision: all three lines get error",
              allError,
              results.size() == 3
                  ? QString("ok=%1%2%3").arg(results[0].ok).arg(results[1].ok).arg(results[2].ok)
                  : "wrong size",
              "ok=000");
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
        // Numi-style labeled assignment: "VAR = (annotation) = value"
        // The parenthesized annotation is stripped; only the numeric expression is evaluated.
        auto r = bridge.evaluateDocument("X = (description) = 100 + 200");
        bool ok = r.size() == 1 && r[0].ok && r[0].hasNumericValue
                  && std::abs(r[0].numericValue - 300.0) < 0.001;
        check("labeled assignment: X = (description) = 100 + 200 gives 300",
              ok, r.size() >= 1 ? r[0].result : "size<1", "300");
    }
    {
        // With places=0, integers show without decimal point.
        auto results = bridge.evaluateDocument("200 + 0");
        bool ok = results.size() == 1 && results[0].ok && results[0].result == "200";
        check("places=0: 200+0 shows as \"200\" (no decimal point)", ok,
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
        auto r = eval("time + 60 min");
        const bool isTime = r.ok && r.result.length() == 8
                         && r.result[2] == ':' && r.result[5] == ':';
        check("time + 60 min returns HH:mm:ss", isTime, r.result, "HH:mm:ss");
    }
    {
        auto r = eval("time - 30 s");
        const bool isTime = r.ok && r.result.length() == 8
                         && r.result[2] == ':' && r.result[5] == ':';
        check("time - 30 s returns HH:mm:ss", isTime, r.result, "HH:mm:ss");
    }
    {
        auto r = eval("time + 2 h");
        const bool isTime = r.ok && r.result.length() == 8
                         && r.result[2] == ':' && r.result[5] == ':';
        check("time + 2 h returns HH:mm:ss", isTime, r.result, "HH:mm:ss");
    }
    {
        // Named-date arithmetic: today ± N days/weeks/months/years → DD.MM.YYYY
        auto r = eval("today + 2 days");
        const QString expected = QDate::currentDate().addDays(2).toString("dd.MM.yyyy");
        check("today + 2 days returns dd.MM.yyyy",
              r.ok && r.result == expected, r.result, expected.toUtf8().constData());
    }
    {
        auto r = eval("today - 1 week");
        const QString expected = QDate::currentDate().addDays(-7).toString("dd.MM.yyyy");
        check("today - 1 week returns dd.MM.yyyy",
              r.ok && r.result == expected, r.result, expected.toUtf8().constData());
    }
    {
        auto r = eval("now + 3 months");
        const QString expected = QDate::currentDate().addMonths(3).toString("dd.MM.yyyy");
        check("now + 3 months returns dd.MM.yyyy",
              r.ok && r.result == expected, r.result, expected.toUtf8().constData());
    }
    {
        auto r = eval("now + 2 days");
        const QString expected = QDate::currentDate().addDays(2).toString("dd.MM.yyyy");
        check("now + 2 days returns dd.MM.yyyy",
              r.ok && r.result == expected, r.result, expected.toUtf8().constData());
    }
    {
        auto r = eval("tomorrow + 5 days");
        const QString expected = QDate::currentDate().addDays(6).toString("dd.MM.yyyy");
        check("tomorrow + 5 days returns dd.MM.yyyy",
              r.ok && r.result == expected, r.result, expected.toUtf8().constData());
    }
    {
        auto r = eval("yesterday - 2 weeks");
        const QString expected = QDate::currentDate().addDays(-1 - 14).toString("dd.MM.yyyy");
        check("yesterday - 2 weeks returns dd.MM.yyyy",
              r.ok && r.result == expected, r.result, expected.toUtf8().constData());
    }
    {
        auto r = eval("today + 1 year");
        const QString expected = QDate::currentDate().addYears(1).toString("dd.MM.yyyy");
        check("today + 1 year returns dd.MM.yyyy",
              r.ok && r.result == expected, r.result, expected.toUtf8().constData());
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
    // ── 2-digit year input (DD.MM.YY → 2000s) ────────────────────────────────
    {
        auto r = eval("01.01.00 + 25 years");
        check("2-digit year 00 → 2000: date plus 25 years = 01.01.2025",
              r.ok && r.result == "01.01.2025",
              r.result, "01.01.2025");
    }
    {
        auto r = eval("01.01.26 - 01.01.25");
        check("2-digit year: 01.01.26 - 01.01.25 returns 1 year span",
              r.ok && r.result.contains("1 year"),
              r.result, "contains '1 year'");
    }
    {
        auto r = eval("05.03.26 + 1 month");
        check("2-digit year: 05.03.26 + 1 month = 05.04.2026",
              r.ok && r.result == "05.04.2026",
              r.result, "05.04.2026");
    }
    {
        auto r = eval("05/03/26 + 1 day");
        check("2-digit year slash separator: 05/03/26 + 1 day = 06.03.2026",
              r.ok && r.result == "06.03.2026",
              r.result, "06.03.2026");
    }
    {
        auto r = eval("05-03-26 + 1 year");
        check("2-digit year dash separator: 05-03-26 + 1 year = 05.03.2027",
              r.ok && r.result == "05.03.2027",
              r.result, "05.03.2027");
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

    // ── Temperature conversion ────────────────────────────────────────────────
    bridge.setDecimalPlaces(2);
    {
        auto r = eval("100 C to F");
        check("100 C to F = 212 °F", r.ok && r.result == "212 °F", r.result, "212 °F");
    }
    {
        auto r = eval("212 F to C");
        check("212 F to C = 100 °C", r.ok && r.result == "100 °C", r.result, "100 °C");
    }
    {
        auto r = eval("0 C to K");
        check("0 C to K = 273.15 K", r.ok && r.result == "273.15 K", r.result, "273.15 K");
    }
    {
        auto r = eval("273.15 K to C");
        check("273.15 K to C = 0 °C", r.ok && r.result == "0 °C", r.result, "0 °C");
    }
    {
        auto r = eval("32 F to K");
        check("32 F to K = 273.15 K", r.ok && r.result == "273.15 K", r.result, "273.15 K");
    }
    {
        auto r = eval("373.15 K to F");
        check("373.15 K to F = 212 °F", r.ok && r.result == "212 °F", r.result, "212 °F");
    }
    {
        // With ° prefix — same result
        auto r = eval("100 °C to F");
        check("100 °C to F (with degree symbol) = 212 °F", r.ok && r.result == "212 °F", r.result, "212 °F");
    }
    bridge.setDecimalPlaces(0);

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
    bridge.setDecimalPlaces(2); // crypto results often fractional; test with 2 places
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

        // ── Scientific notation in currency expressions ───────────────────────
        // rates: BTC=50000 USD, ETH=2500 USD, EUR=1.25 USD. decimalPlaces=2.
        {
            // 0.001 BTC * 50000 USD/BTC = 50 USD
            auto r = eval("1e-3 BTC to USD");
            check("1e-3 BTC to USD = USD 50 (scientific notation)",
                  r.ok && r.result == "USD 50", r.result, "USD 50");
        }
        {
            // uppercase E: 100 BTC * 50000 = 5,000,000 USD
            const QString expected = QStringLiteral("USD %1").arg(QLocale().toString(5000000));
            auto r = eval("1E2 BTC to USD");
            check("1E2 BTC to USD (uppercase E) = USD 5,000,000",
                  r.ok && r.result == expected, r.result, expected.toUtf8().constData());
        }
        {
            // 150 USD / 1.25 USD/EUR = 120 EUR
            auto r = eval("1.5e2 USD to EUR");
            check("1.5e2 USD to EUR = EUR 120",
                  r.ok && r.result == "EUR 120", r.result, "EUR 120");
        }
        {
            // mixed crypto: 0.001 BTC + 0.001 ETH → USD (default currency)
            // 0.001 * 50000 + 0.001 * 2500 = 50 + 2.5 = 52.5 USD
            auto r = eval("1e-3 BTC + 1e-3 ETH");
            check("1e-3 BTC + 1e-3 ETH = USD 52.5 (scientific notation, mixed crypto)",
                  r.ok && r.result == "USD 52.5", r.result, "USD 52.5");
        }
        {
            // subtraction: 1e-3 BTC - 1e-4 BTC = 0.0009 BTC = 45 USD
            auto r = eval("1e-3 BTC - 1e-4 BTC to USD");
            check("1e-3 BTC - 1e-4 BTC to USD = USD 45",
                  r.ok && r.result == "USD 45", r.result, "USD 45");
        }
    }
    bridge.setDecimalPlaces(3); // switch to 3 places for math functions with irrational results

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
    bridge.setDecimalPlaces(0); // reset

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
        bridge.setDecimalPlaces(1); // 0.5 needs at least 1 decimal place to display correctly
        auto results = bridge.evaluateDocument("2 : 4");
        check("colon division support (2 : 4 = 0.5)",
              results.size() == 1 && results[0].result == "0.5", results.size() >= 1 ? results[0].result : "error", "0.5");
        bridge.setDecimalPlaces(0);
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
        bridge.setDecimalPlaces(3); // need 3 places to preserve the .414 part
        auto results = bridge.evaluateDocument("7,486,332.414 + 1");
        check("robust thousands separator removal",
              results.size() == 1 && results[0].result == QLocale().toString(7486333.414, 'f', 3),
              results.size() >= 1 ? results[0].result : "error", "7,486,333.414");
        bridge.setDecimalPlaces(0);
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
        // uses places=0 so result is "USD 525" without decimal point
        bridge.setDecimalPlaces(0);
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
        // "today - 30 da": separator is '-', after it " 30 da" → last word = "da" → matches "days"
        auto list = bridge.getCompletions("today - 30 da");
        check("getCompletions extracts last word 'da' from 'today - 30 da'",
              !list.isEmpty() && list[0].section('\t', 0, 0).toLower().startsWith("da"),
              list.isEmpty() ? "empty" : list[0].section('\t', 0, 0), "days");
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
    {
        // "seconds" is now a keyword: "260 seconds" → prefix after separator = none,
        // segment = "260 seconds", lastWord = "seconds"
        auto list = bridge.getCompletions("260 seconds");
        check("getCompletions 'seconds' keyword matched in '260 seconds'",
              !list.isEmpty() && list[0].section('\t', 0, 0) == "seconds",
              list.isEmpty() ? "empty" : list[0].section('\t', 0, 0), "seconds");
    }
    {
        // "minutes" keyword
        auto list = bridge.getCompletions("30 min");
        check("getCompletions 'min' matches 'minutes' keyword",
              !list.isEmpty() && list[0].section('\t', 0, 0).startsWith("min"),
              list.isEmpty() ? "empty" : list[0].section('\t', 0, 0), "minutes");
    }
    {
        // "hours" keyword
        auto list = bridge.getCompletions("2 ho");
        check("getCompletions 'ho' matches 'hours' keyword",
              !list.isEmpty() && list[0].section('\t', 0, 0) == "hours",
              list.isEmpty() ? "empty" : list[0].section('\t', 0, 0), "hours");
    }
    {
        // Multi-word variable completion: "Что такое " (trailing space) matches
        // a variable defined as "Что такое переменная".
        bridge.evaluateDocument(QString::fromUtf8("Что такое переменная = 1000"));
        auto list = bridge.getCompletions(QString::fromUtf8("Что такое "));
        check("getCompletions matches multi-word variable on trailing-space prefix",
              !list.isEmpty() && list[0].section('\t', 0, 0) == QString::fromUtf8("Что такое переменная"),
              list.isEmpty() ? "empty" : list[0].section('\t', 0, 0),
              "Что такое переменная");
    }
    {
        // Compound unit result should have no forced .00 when places=2
        bridge.setDecimalPlaces(2);
        auto r = eval("260 seconds");
        check("260 seconds compound result has no .00 trailing zeros (places=2)",
              r.ok && !r.result.isEmpty() && !r.result.contains(".00"),
              r.result, "4 min + 20 s");
        bridge.setDecimalPlaces(0);
    }
}

#include <QSignalSpy>
#include <QTest>

static void runDocumentModelSuite() {
    DocumentModel model;
    QSignalSpy spy(&model, &DocumentModel::linesChanged);

    // spy must be cleared BEFORE setSource to avoid consuming the signal
    // that fires between setSource() and spy.wait().
    auto setAndWait = [&](const QString &src) {
        spy.clear();
        model.setSource(src);
        // 5 s budget; covers slow CI machines where currency/unit I/O takes > 2 s
        spy.wait(5000);
    };

    // ── DocumentModel totals ─────────────────────────────────────────────────
    {
        setAndWait("100\n200\n300");
        bool ok = model.rowCount() == 3
               && model.resultCount() == 3
               && model.errorCount() == 0
               && model.hasTotal()
               && model.total() == 600.0;
        check("DocumentModel total = 600", ok,
              QString::number(model.total()), "600");
    }
    {
        setAndWait("1 km to m\n2 km to m");
        bool ok = model.rowCount() == 2
               && model.resultCount() == 2
               && model.errorCount() == 0
               && model.hasTotal()
               && model.total() == 3000.0;
        check("DocumentModel total includes converted unit results", ok,
              QString::number(model.total()), "3000");
    }
    {
        setAndWait("500 AED to USD\n100");
        bool ok = model.rowCount() == 2
               && model.resultCount() == 2
               && model.errorCount() == 0
               && !model.hasTotal();
        check("DocumentModel total rejects mixed dimensions", ok,
              model.hasTotal() ? QString::number(model.total()) : "no total", "no total");
    }
    {
        setAndWait("500 AED to USD\n100 USD to UAH");
        bool ok = model.rowCount() == 2
               && model.resultCount() == 2
               && model.errorCount() == 0
               && !model.hasTotal();
        check("DocumentModel total rejects different currency targets", ok,
              model.hasTotal() ? QString::number(model.total()) : "no total", "no total");
    }
    {
        // Bug: "X AED to EUR" got totalKey="EUR" but "X EUR" got totalKey="number",
        // so they were treated as different dimensions and hasTotal() returned false.
        setAndWait("200 AED to EUR\n60 EUR");
        bool ok = model.rowCount() == 2
               && model.resultCount() == 2
               && model.errorCount() == 0
               && model.hasTotal();
        check("DocumentModel total: conversion-to-EUR + plain-EUR sums correctly", ok,
              model.hasTotal() ? "has total" : "no total", "has total");
    }
    {
        const QString date = QDate::currentDate().addYears(-2).addDays(-5).toString("dd.MM.yyyy");
        setAndWait(QStringLiteral("today - %1\n100").arg(date));
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
        // AED/USD are currency entities → yellow #ffd35a; "to" is a keyword → yellow #ffd35a
        const QString html = model.highlightExample("500 AED to USD");
        bool ok = html.contains("#ffd35a") && html.contains("AED")
               && html.contains("USD")   && html.contains("to");
        check("DocumentModel help examples use shared syntax highlighting", ok,
              html, "highlighted AED/USD/to in yellow");
    }

    // ── getCompletions via DocumentModel ─────────────────────────────────────
    {
        // After evaluating a doc with a variable, completion finds it
        setAndWait("myvar = 42");
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

    // ── Generation ID: rapid source changes ──────────────────────────────────
    {
        // Set source three times without waiting. The final linesChanged signal
        // must reflect "3 + 3" (= 6), not any earlier evaluation.
        if (!spy.isEmpty()) spy.clear();
        model.setSource("1 + 1");
        model.setSource("2 + 2");
        model.setSource("3 + 3");
        spy.wait(2000);
        const QString result = model.data(model.index(0), DocumentModel::ResultRole).toString();
        // Result may include decimals depending on current decimalPlaces setting (e.g. "6.000").
        check("async: rapid source changes yield result for last source",
              result.startsWith("6"), result, "6*");
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

    // ── Tab replaces only the current word, not the number before it ─────────
    // Regression: "200 - 30 da" + Tab → _getWordStart must return position of 'd',
    // not '3'. Without last-word fix, "30 da" was removed instead of just "da".
    {
        setText("100\n200 - 30 da");
        QTest::keyClick(&view, Qt::Key_Tab);
        QTest::qWait(80);
        QString text = root->property("text").toString();
        check("Tab with 'N WORD' pattern replaces only the word, not the number before it",
              text.startsWith("100\n") && text.contains("200 - 30 ") && !text.contains("200 - days"),
              QString("got: \"%1\"").arg(text.left(50)), "100\\n200 - 30 days");
    }

    // ── Tab on non-first line must NOT erase preceding lines ─────────────────
    // Regression: _getWordStart() returned 0 (absolute) when no separator was
    // found on the current line (sepIdx=-1 → start=sepIdx+1=0).
    // editor.remove(0, cursor) then erased the entire preceding content.
    {
        setText("100\n200\nkil");
        QTest::keyClick(&view, Qt::Key_Tab);
        QTest::qWait(80);
        QString text = root->property("text").toString();
        check("Tab completion on line 3 preserves lines 1 and 2 (regression: wordStart=0 erasure)",
              text.startsWith("100\n200\n"),
              QString("got: \"%1\"").arg(text.left(40)), "100\\n200\\n...");
    }
    {
        // Verify the completion was actually inserted on the correct line
        setText("100\n200\nkil");
        QTest::keyClick(&view, Qt::Key_Tab);
        QTest::qWait(80);
        QString text = root->property("text").toString();
        QString thirdLine = text.section('\n', 2, 2);
        check("Tab completion on line 3 inserts the completion on line 3",
              !thirdLine.isEmpty() && thirdLine != "kil",
              QString("line3: \"%1\"").arg(thirdLine), "kilogram/kilometer/kilowatt");
    }

    // ── Multi-word variable completion via trailing space ────────────────────
    // "Var name " + Tab → getCompletions returns "Var name extended\t1000"
    // → should replace the full segment "Var name " with "Var name extended"
    {
        setText("Var name ");
        QTest::keyClick(&view, Qt::Key_Tab);
        QTest::qWait(80);
        QString text = root->property("text").toString().trimmed();
        check("Tab with trailing space completes multi-word variable (replaces full segment)",
              text == "Var name extended",
              QString("got: \"%1\"").arg(text), "Var name extended");
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
