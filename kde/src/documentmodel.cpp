#include "documentmodel.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonValue>
#include <QProcess>

#ifndef NUMI_KDE_SOURCE_DIR
#define NUMI_KDE_SOURCE_DIR ""
#endif

DocumentModel::DocumentModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

QString DocumentModel::source() const
{
    return m_source;
}

void DocumentModel::setSource(const QString &source)
{
    if (m_source == source) {
        return;
    }

    m_source = source;
    emit sourceChanged();
    evaluate();
}

int DocumentModel::errorCount() const
{
    return m_errorCount;
}

int DocumentModel::resultCount() const
{
    return m_resultCount;
}

int DocumentModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_lines.size();
}

QVariant DocumentModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_lines.size()) {
        return {};
    }

    const auto line = m_lines.at(index.row()).toObject();
    switch (role) {
    case LineRole:
        return line.value(QStringLiteral("number")).toInt();
    case ResultRole:
        return line.value(QStringLiteral("result")).toString();
    case OkRole:
        return line.value(QStringLiteral("ok")).toBool();
    case DiagnosticRole:
        return firstDiagnostic(line);
    case HighlightedHtmlRole:
        return line.value(QStringLiteral("highlightedHtml")).toString();
    default:
        return {};
    }
}

QHash<int, QByteArray> DocumentModel::roleNames() const
{
    return {
        {LineRole, "line"},
        {ResultRole, "result"},
        {OkRole, "ok"},
        {DiagnosticRole, "diagnostic"},
        {HighlightedHtmlRole, "highlightedHtml"},
    };
}

void DocumentModel::copyResult(int row)
{
    if (row < 0 || row >= m_lines.size()) {
        return;
    }

    const auto result = m_lines.at(row).toObject().value(QStringLiteral("result")).toString();
    if (!result.isEmpty()) {
        QGuiApplication::clipboard()->setText(result);
    }
}

void DocumentModel::evaluate()
{
    const auto output = runEvaluator(m_source);
    const auto document = QJsonDocument::fromJson(output.toUtf8());
    const auto root = document.object();
    const auto summary = root.value(QStringLiteral("summary")).toObject();
    const auto lines = root.value(QStringLiteral("lines")).toArray();

    beginResetModel();
    m_lines = lines;
    m_errorCount = summary.value(QStringLiteral("errorCount")).toInt();
    m_resultCount = summary.value(QStringLiteral("resultCount")).toInt();
    endResetModel();

    emit linesChanged();
}

QString DocumentModel::runEvaluator(const QString &source)
{
    QProcess process;
    process.setProgram(QStringLiteral("node"));
    process.setArguments({QStringLiteral(NUMI_KDE_SOURCE_DIR) + QStringLiteral("/src/gui/evaluate-document.js")});
    process.start();
    if (!process.waitForStarted(1000)) {
        return QStringLiteral("{\"lines\":[],\"summary\":{\"errorCount\":1,\"resultCount\":0}}");
    }

    process.write(source.toUtf8());
    process.closeWriteChannel();
    process.waitForFinished(2000);
    return QString::fromUtf8(process.readAllStandardOutput());
}

QString DocumentModel::firstDiagnostic(const QJsonObject &line)
{
    const auto diagnostics = line.value(QStringLiteral("diagnostics")).toArray();
    if (diagnostics.isEmpty()) {
        return {};
    }
    return diagnostics.first().toObject().value(QStringLiteral("message")).toString();
}
