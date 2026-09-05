#include "engineprotocol.h"

#include <QJsonDocument>

namespace EngineProtocol {

QJsonObject toJson(const LineResult &line)
{
    QJsonObject o;
    o.insert(QStringLiteral("ok"), line.ok);
    if (!line.result.isEmpty()) o.insert(QStringLiteral("result"), line.result);
    if (!line.error.isEmpty()) o.insert(QStringLiteral("error"), line.error);
    if (!line.highlightedHtml.isEmpty()) o.insert(QStringLiteral("html"), line.highlightedHtml);
    if (line.hasNumericValue) o.insert(QStringLiteral("value"), line.numericValue);
    if (!line.totalKey.isEmpty()) o.insert(QStringLiteral("totalKey"), line.totalKey);
    return o;
}

LineResult lineFromJson(const QJsonObject &o)
{
    LineResult line;
    line.ok = o.value(QStringLiteral("ok")).toBool();
    line.result = o.value(QStringLiteral("result")).toString();
    line.error = o.value(QStringLiteral("error")).toString();
    line.highlightedHtml = o.value(QStringLiteral("html")).toString();
    if (o.contains(QStringLiteral("value"))) {
        line.hasNumericValue = true;
        line.numericValue = o.value(QStringLiteral("value")).toDouble();
    }
    line.totalKey = o.value(QStringLiteral("totalKey")).toString();
    return line;
}

QJsonArray toJson(const QList<LineResult> &lines)
{
    QJsonArray a;
    for (const LineResult &l : lines) a.append(toJson(l));
    return a;
}

QList<LineResult> linesFromJson(const QJsonArray &a)
{
    QList<LineResult> lines;
    lines.reserve(a.size());
    for (const QJsonValue &v : a) lines.append(lineFromJson(v.toObject()));
    return lines;
}

QByteArray encode(const QJsonObject &message)
{
    return QJsonDocument(message).toJson(QJsonDocument::Compact) + '\n';
}

} // namespace EngineProtocol
