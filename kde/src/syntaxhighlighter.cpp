#include "syntaxhighlighter.h"
#include <libqalculate/qalculate.h>

SyntaxHighlighter::SyntaxHighlighter(Calculator *calc) : m_calc(calc) {
    // Ported regex from highlight.js
    m_tokenRegex = QRegularExpression(
        "\\s+|\\d{1,3}(?:,\\d{3})+(?:\\.\\d+)?(?:e[+-]?\\d+)?|\\d+(?:[.,]\\d+)?(?:e[+-]?\\d+)?|[A-Za-z_π\\p{L}][\\wπ\\p{L}]*|:=|[%+\\-*/^(),;=]|\\S",
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption
    );

    m_operatorWords = {"as", "from", "in", "of", "now", "time", "today", "to"};
    m_operatorSymbols = {":=", "+", "-", "^", "(", ")", ",", ";", "="};
}

QString SyntaxHighlighter::highlightLine(const QString &line, const QSet<QString> &variables) {
    if (line.trimmed().startsWith("#")) {
        return QString("<span style=\"color:#22d3ee\">%1</span>").arg(escapeHtml(line));
    }

    QString result;
    auto it = m_tokenRegex.globalMatch(line);

    while (it.hasNext()) {
        auto match = it.next();
        QString token = match.captured(0);

        if (token.trimmed().isEmpty()) {
            result += escapeWhitespace(token);
            continue;
        }

        QString kind = classify(token, variables);
        QString escaped = escapeHtml(token);

        if (kind == "variable") {
            result += QString("<span style=\"color:#39ff14\">%1</span>").arg(escaped);
        } else if (kind == "operator") {
            result += QString("<span style=\"color:#ffd35a\">%1</span>").arg(escaped);
        } else if (kind == "entity") {
            result += QString("<span style=\"color:#6fc4e8\">%1</span>").arg(escaped);
        } else {
            result += escaped;
        }
    }

    return result;
}

QString SyntaxHighlighter::classify(const QString &token, const QSet<QString> &variables) {
    if (variables.contains(token)) {
        return "variable";
    }

    if (m_classificationCache.contains(token)) {
        return m_classificationCache.value(token);
    }

    QString lower = token.toLower();
    QString kind;

    if (m_operatorWords.contains(lower) || m_operatorSymbols.contains(token)) {
        kind = "operator";
    } else if (token == "%" || token == "*" || token == "/") {
        kind = "entity";
    } else {
        // Check for currency (uppercase, 3-5 chars)
        static QRegularExpression currencyRegex("^[A-Z]{3,5}$");
        if (currencyRegex.match(token).hasMatch()) {
            kind = "entity";
        } else if (m_calc) {
            // Check for unit using libqalculate (try original and uppercase for lowercase currency tokens)
            if (m_calc->getUnit(token.toStdString()) ||
                (!m_operatorWords.contains(lower) && m_calc->getUnit(token.toUpper().toStdString()))) {
                kind = "entity";
            }
        }
    }

    m_classificationCache.insert(token, kind);
    return kind;
}

QString SyntaxHighlighter::escapeHtml(const QString &input) {
    QString res = input;
    res.replace("&", "&amp;");
    res.replace("<", "&lt;");
    res.replace(">", "&gt;");
    res.replace("\"", "&quot;");
    return res;
}

QString SyntaxHighlighter::escapeWhitespace(const QString &input) {
    QString res = input;
    res.replace("\t", "&nbsp;&nbsp;&nbsp;&nbsp;");
    res.replace(" ", "&nbsp;");
    return res;
}
