/**
 * Portions Copyright (c) Nikolas
 * Original MIT License: https://github.com/nikolaeu/numi/blob/master/license.txt
 * Modified by DMYTRO SKORIN under Apache License 2.0
 */

#include "syntaxhighlighter.h"
#include <libqalculate/qalculate.h>
#include <algorithm>

static const char *USER_VAR_COLOR = "#fb923c";

SyntaxHighlighter::SyntaxHighlighter(Calculator *calc) : m_calc(calc) {
    // Ported regex from highlight.js
    m_tokenRegex = QRegularExpression(
        "\\s+|\\d{1,3}(?:,\\d{3})+(?:\\.\\d+)?(?:e[+-]?\\d+)?|\\d+(?:[.,]\\d+)?(?:e[+-]?\\d+)?|[A-Za-z_π\\p{L}][\\wπ\\p{L}]*|:=|[%+\\-*/^(),;=]|\\S",
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption
    );

    m_operatorWords = {"as", "from", "in", "of", "now", "time", "today", "to"};
    m_operatorSymbols = {":=", "+", "-", "^", "(", ")", ",", ";", "="};
}

QString SyntaxHighlighter::highlightLine(const QString &line,
                                          const QSet<QString> &singleWordVars,
                                          const QStringList &multiWordVarsSorted)
{
    if (line.trimmed().startsWith("#")) {
        return QString("<span style=\"color:#22d3ee\">%1</span>").arg(escapeHtml(line));
    }

    if (multiWordVarsSorted.isEmpty()) {
        return tokenizeSegment(line, singleWordVars);
    }

    // Find all non-overlapping occurrences of multi-word variable names (longest first).
    struct Span { int start, len; };
    QVector<Span> spans;

    for (const QString &varName : multiWordVarsSorted) {
        int idx = 0;
        while (idx < line.length()) {
            int found = line.indexOf(varName, idx, Qt::CaseSensitive);
            if (found == -1) break;
            bool overlap = false;
            for (const Span &s : spans) {
                if (found < s.start + s.len && found + varName.length() > s.start) {
                    overlap = true;
                    break;
                }
            }
            if (!overlap)
                spans.append({found, (int)varName.length()});
            idx = found + varName.length();
        }
    }

    if (spans.isEmpty()) {
        return tokenizeSegment(line, singleWordVars);
    }

    std::sort(spans.begin(), spans.end(), [](const Span &a, const Span &b) {
        return a.start < b.start;
    });

    QString result;
    int pos = 0;
    for (const Span &sp : spans) {
        if (sp.start > pos)
            result += tokenizeSegment(line.mid(pos, sp.start - pos), singleWordVars);
        result += QString("<span style=\"color:%1\">%2</span>")
                  .arg(QLatin1String(USER_VAR_COLOR))
                  .arg(escapeHtml(line.mid(sp.start, sp.len)));
        pos = sp.start + sp.len;
    }
    if (pos < line.length())
        result += tokenizeSegment(line.mid(pos), singleWordVars);

    return result;
}

QString SyntaxHighlighter::tokenizeSegment(const QString &text, const QSet<QString> &variables)
{
    QString result;
    auto it = m_tokenRegex.globalMatch(text);

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
            result += QString("<span style=\"color:%1\">%2</span>")
                      .arg(QLatin1String(USER_VAR_COLOR))
                      .arg(escaped);
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
