#include "qalcbridge.h"
#include <libqalculate/qalculate.h>
#include <QStringList>

QalcBridge::QalcBridge(QObject *parent) : QObject(parent) {
    m_calc = new Calculator();
    m_calc->loadGlobalDefinitions();
    m_calc->loadExchangeRates(); // This might be slow/sync, but let's start with it
}

QalcBridge::~QalcBridge() {
    delete m_calc;
}

QList<LineResult> QalcBridge::evaluateDocument(const QString &source) {
    QList<LineResult> results;
    QStringList lines = source.split('\n');

    EvaluationOptions eo;
    eo.parse_options.angle_unit = ANGLE_UNIT_RADIANS;
    eo.structuring = STRUCTURING_SIMPLIFY;

    PrintOptions po;
    po.number_fraction_format = FRACTION_DECIMAL;
    po.base = 10;

    for (const QString &line : lines) {
        LineResult res;
        res.ok = false;

        if (line.trimmed().isEmpty() || line.trimmed().startsWith("//") || line.trimmed().startsWith("#")) {
            res.ok = true;
            res.result = "";
            results.append(res);
            continue;
        }

        try {
            MathStructure result = m_calc->calculate(line.toStdString(), eo);
            res.result = QString::fromStdString(m_calc->print(result, 2000, po));
            res.ok = true;
        } catch (...) {
            res.ok = false;
            res.error = "Calculation error";
        }

        results.append(res);
    }

    return results;
}
