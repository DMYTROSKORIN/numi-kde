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
#include <cstdio>

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
              r.ok && r.result.startsWith("2 years and 5 days (excluding the end date).\n"),
              r.result, "2 years and 5 days (excluding the end date).");
    }
    {
        auto r = eval("26.08.1983 - 02.05.2026");
        const QString expected = QStringLiteral("42 years, 8 months and 6 days (excluding the end date).\n%1 days.")
            .arg(QLocale(QLocale::English, QLocale::UnitedStates).toString(QDate(1983, 8, 26).daysTo(QDate(2026, 5, 2))));
        check("date minus date returns detailed English span",
              r.ok && r.result == expected,
              r.result, expected.toUtf8().constData());
    }
    {
        auto r = eval("26.08.1983 + 42 years");
        check("date plus years returns date",
              r.ok && r.result == "26.08.2025",
              r.result, "26.08.2025");
    }
    {
        auto r = eval("today - 26.08.1983");
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
        check("fiat conversion exposes numeric value",
              r.ok && r.hasNumericValue && r.numericValue > 0.0 && r.totalKey == "USD",
              QString("%1 (%2)").arg(r.result, QString::number(r.numericValue)), "numeric conversion");
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
        check("1 BTC to EUR includes target currency prefix",
              btcToEur.ok && btcToEur.result.startsWith("EUR ") && btcToEur.hasNumericValue && btcToEur.numericValue > 0.0,
              btcToEur.result, "EUR <number>");

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
        QString c = bridge.getCompletion("me");
        check("completion 'me' non-empty", !c.isEmpty(), c, "non-empty");
    }
}

static void runDocumentModelSuite() {
    DocumentModel model;

    // ── DocumentModel totals ─────────────────────────────────────────────────
    {
        model.setSource("100\n200\n300");
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
        bool ok = model.rowCount() == 2
               && model.resultCount() == 2
               && model.errorCount() == 0
               && !model.hasTotal();
        check("DocumentModel total rejects mixed dimensions", ok,
              model.hasTotal() ? QString::number(model.total()) : "no total", "no total");
    }
    {
        model.setSource("500 AED to USD\n100 USD to UAH");
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
    app.setOrganizationName("skorin");

    const QString suite = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("qalc");
    std::printf("=== numi-kde %s Test Suite ===\n\n",
                suite == QStringLiteral("documentmodel") ? "DocumentModel" : "QalcBridge");

    if (suite == QStringLiteral("documentmodel")) {
        runDocumentModelSuite();
    } else {
        QalcBridge bridge;
        bridge.setDecimalPlaces(3);

        runSuite(bridge);
    }

    std::printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
