#pragma once

#include "lineresult.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QList>

/**
 * Wire format between numi-kde (GUI) and numi-kde-engine (libqalculate host).
 *
 * One compact JSON object per line, UTF-8, over the engine's stdin/stdout.
 *
 * Requests (GUI → engine), all carry "op" and an integer "id":
 *   configure   {decimals, currency}
 *   evaluate    {source, skip:[line indices to report as timed out]}
 *   cancel      {target: id of the evaluate to abandon}
 *   completions {context}      → reply {items:[...]}
 *   completion  {prefix}       → reply {value}
 *   highlight   {line}         → reply {html}
 *   ping                       → reply {ok:true}
 * Replies carry the request "id". An evaluate additionally streams
 *   {ev:"progress", id, index}  after every finished line, then
 *   {id, lines:[LineResult...]}.
 * Unsolicited events: {ev:"ready", version}, {ev:"ratesUpdated"},
 *   {ev:"networkStatus", value}.
 */
namespace EngineProtocol {

QJsonObject toJson(const LineResult &line);
LineResult lineFromJson(const QJsonObject &object);
QJsonArray toJson(const QList<LineResult> &lines);
QList<LineResult> linesFromJson(const QJsonArray &array);

/// Serialises one message as a single line (compact JSON + '\n').
QByteArray encode(const QJsonObject &message);

} // namespace EngineProtocol
