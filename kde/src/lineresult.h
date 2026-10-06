#pragma once

#include <QString>

// totalKey of a dimensionless result (plain arithmetic). Currency and unit
// results carry their code instead (EUR, USD, M, ...). Plain numbers join a
// single typed group in the Total footer; two different typed groups hide it.
inline const QString kPlainTotalKey = QStringLiteral("number");

// One evaluated document line. Produced by QalcBridge (engine process),
// transported as JSON by EngineProtocol, consumed by DocumentModel (GUI).
struct LineResult {
    bool ok = false;
    QString result;
    QString error;
    QString highlightedHtml;
    bool hasNumericValue = false;
    double numericValue = 0.0;
    QString totalKey;
};
