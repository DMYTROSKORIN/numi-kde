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
    po.min_decimals = m_decimalPlaces;
    po.use_max_decimals = true;
    po.use_min_decimals = true;

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

        // Pre-process: libqalculate functional fixes
        QString processedLine = line;
        
        // 1. Map "now" to "today()" to ensure date arithmetic works
        static QRegularExpression nowRegex("\\bnow\\b", QRegularExpression::CaseInsensitiveOption);
        processedLine.replace(nowRegex, "today()");

        // 2. Map "to sq" to "to sqm" (Numi compatibility)
        static QRegularExpression sqRegex("\\b(to|in)\\s+sq\\b", QRegularExpression::CaseInsensitiveOption);
        processedLine.replace(sqRegex, "\\1 sqm");

        // 3. Percentage fix: libqalculate prefers "X% of Y" over "X% from Y"
        static QRegularExpression fromRegex("(\\d+%)\\s+from\\s+", QRegularExpression::CaseInsensitiveOption);
        processedLine.replace(fromRegex, "\\1 of ");

        // 4. Case-insensitive currency: Uppercase standalone 3-letter words
        static QRegularExpression currencyRegex("\\b([a-z]{3})\\b");
        auto currIt = currencyRegex.globalMatch(processedLine);
        int offset = 0;
        while (currIt.hasNext()) {
            auto match = currIt.next();
            QString replacement = match.captured(1).toUpper();
            processedLine.replace(match.capturedStart(1) + offset, match.capturedLength(1), replacement);
            // In this case offset is 0 because length of 3-letter currency doesn't change
        }

        // 5. Variable assignment logic: Evaluate RHS then assign result to LHS
        auto assignMatch = assignmentRegex.match(processedLine.trimmed());
        if (assignMatch.hasMatch()) {
            QString varName = assignMatch.captured(1);
            QString rhs = assignMatch.captured(2);
            try {
                // Evaluate RHS
                MathStructure rhsResult = m_calc->calculate(rhs.toStdString(), eo);
                QString rhsString = QString::fromStdString(m_calc->print(rhsResult, 2000, po));
                
                // Assign result to variable
                m_calc->calculate(QString("%1 := %2").arg(varName, rhsString).toStdString(), eo);
                
                res.result = rhsString;
                res.ok = true;
            } catch (...) {
                res.ok = false;
                res.error = "Assignment error";
            }
        } else {
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
        }

        results.append(res);
    }

    return results;
}

QString QalcBridge::getCompletion(const QString &prefix) {
    if (prefix.isEmpty()) return prefix;

    QStringList matches;
    std::string p = prefix.toLower().toStdString();

    // Check Units
    for (size_t i = 0; ; ++i) {
        Unit *u = m_calc->getUnit(i);
        if (!u) break;
        for (size_t j = 0; j < u->countNames(); ++j) {
            const std::string &name = u->getName(j).name;
            if (name.find(p) == 0) matches << QString::fromStdString(name);
        }
    }

    // Check Variables
    for (size_t i = 0; ; ++i) {
        Variable *v = m_calc->getVariable(i);
        if (!v) break;
        for (size_t j = 0; j < v->countNames(); ++j) {
            const std::string &name = v->getName(j).name;
            if (name.find(p) == 0) matches << QString::fromStdString(name);
        }
    }

    // Check Functions
    for (size_t i = 0; ; ++i) {
        MathFunction *f = m_calc->getFunction(i);
        if (!f) break;
        for (size_t j = 0; j < f->countNames(); ++j) {
            const std::string &name = f->getName(j).name;
            if (name.find(p) == 0) matches << QString::fromStdString(name);
        }
    }

    // Check custom keywords
    QStringList keywords = {"today", "tomorrow", "yesterday", "now", "days", "weeks", "months", "years"};
    for (const auto &kw : keywords) {
        if (kw.startsWith(prefix, Qt::CaseInsensitive)) matches << kw;
    }

    matches.removeDuplicates();
    if (matches.isEmpty()) return prefix;
    if (matches.size() == 1) return matches.first();

    // Find longest common prefix
    QString common = matches.first();
    for (int i = 1; i < matches.size(); ++i) {
        const QString &s = matches.at(i);
        int j = 0;
        while (j < common.length() && j < s.length() && common[j].toLower() == s[j].toLower()) {
            j++;
        }
        common = common.left(j);
    }

    return common.isEmpty() ? prefix : common;
}
