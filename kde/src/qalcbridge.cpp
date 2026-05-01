#include "qalcbridge.h"
#include "syntaxhighlighter.h"
#include <libqalculate/qalculate.h>
#include <QStringList>
#include <QRegularExpression>
#include <QSet>

QalcBridge::QalcBridge(QObject *parent) : QObject(parent) {
    m_calc = new Calculator();
    m_calc->loadGlobalDefinitions();
    m_calc->loadExchangeRates();
    m_highlighter = new SyntaxHighlighter(m_calc);
}

QalcBridge::~QalcBridge() {
    delete m_highlighter;
    delete m_calc;
}

void QalcBridge::setDecimalPlaces(int places) {
    m_decimalPlaces = places;
}

QList<LineResult> QalcBridge::evaluateDocument(const QString &source) {
    QList<LineResult> results;
    QStringList lines = source.split('\n');

    // First pass: collect variable names for highlighting
    QSet<QString> variables;
    static QRegularExpression assignmentRegex("^([A-Za-z_π\\p{L}][\\wπ\\p{L}]*)\\s*(?::=|=)\\s*(.+)$", QRegularExpression::UseUnicodePropertiesOption);
    for (const QString &line : lines) {
        auto match = assignmentRegex.match(line.trimmed());
        if (match.hasMatch()) {
            variables.insert(match.captured(1));
        }
    }

    EvaluationOptions eo;
    eo.parse_options.angle_unit = ANGLE_UNIT_RADIANS;
    eo.structuring = STRUCTURING_SIMPLIFY;

    PrintOptions po;
    po.number_fraction_format = FRACTION_DECIMAL;
    po.base = 10;
    po.max_decimals = m_decimalPlaces;

    for (const QString &line : lines) {
        LineResult res;
        res.ok = false;
        QString trimmed = line.trimmed();

        res.highlightedHtml = m_highlighter->highlightLine(line, variables);

        if (trimmed.isEmpty() || trimmed.startsWith("//") || trimmed.startsWith("#")) {
            res.ok = true;
            res.result = "";
            results.append(res);
            continue;
        }

        if (trimmed == "/help") {
            res.ok = true;
            res.result = "Numi-KDE Help:\n"
                         "• Math: 2 + 2 * 3^2, sqrt(256), sin(pi/2)\n"
                         "• Units: 10 meters in feet, 50kg to lbs\n"
                         "• Currency: 100 USD to EUR, 50 EUR in JPY\n"
                         "• Dates: today + 2 weeks, 2026-05-15 - today\n"
                         "• Variables: x = 10, y := x * 2\n"
                         "• Percentages: 20% of 200, 20% from 200";
            results.append(res);
            continue;
        }

        // Pre-process: libqalculate prefers "X% of Y" over "X% from Y"
        QString processedLine = line;
        static QRegularExpression fromRegex("(\\d+%)\\s+from\\s+", QRegularExpression::CaseInsensitiveOption);
        processedLine.replace(fromRegex, "\\1 of ");

        try {
            MathStructure result = m_calc->calculate(processedLine.toStdString(), eo);
            QString out = QString::fromStdString(m_calc->print(result, 2000, po));
            // Strip quotes from dates/strings
            if (out.startsWith('"') && out.endsWith('"')) {
                out = out.mid(1, out.length() - 2);
            }
            res.result = out;
            res.ok = true;
        } catch (...) {
            res.ok = false;
            res.error = "Calculation error";
        }

        results.append(res);
    }

    return results;
}
