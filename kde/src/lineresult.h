#pragma once

#include <QString>

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
