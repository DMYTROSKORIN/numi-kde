#include "documentmodel.h"
#include "qalcbridge.h"

#include <KWindowSystem>
#include <KX11Extras>
#include <netwm.h>
#include <QClipboard>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonValue>
#include <QProcess>
#include <QWindow>

#ifndef NUMI_KDE_SOURCE_DIR
#define NUMI_KDE_SOURCE_DIR ""
#endif

DocumentModel::DocumentModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_qalc = new QalcBridge(this);
}

DocumentModel::~DocumentModel()
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

void DocumentModel::setKeepAbove(bool above)
{
    if (auto *win = qApp->focusWindow()) {
        if (KWindowSystem::isPlatformX11()) {
            KX11Extras::setState(win->winId(), above ? NET::KeepAbove : NET::States());
        } else {
            // On Wayland, Qt.WindowStaysOnTopHint should be enough.
            // It is already handled by QML property binding to root.flags.
            win->setFlag(Qt::WindowStaysOnTopHint, above);
        }
    }
}

void DocumentModel::evaluate()
{
    const auto highlightOutput = runHighlightWorker(m_source);
    const auto highlightDoc = QJsonDocument::fromJson(highlightOutput.toUtf8());
    const auto highlightLines = highlightDoc.object().value(QStringLiteral("lines")).toArray();

    m_qalc->setDecimalPlaces(m_qalc->parent()->property("decimalPlaces").toInt());
    const auto qalcResults = m_qalc->evaluateDocument(m_source);

    beginResetModel();
    m_lines = QJsonArray();
    m_errorCount = 0;
    m_resultCount = 0;

    for (int i = 0; i < qalcResults.size(); ++i) {
        QJsonObject lineObj;
        const auto &res = qalcResults.at(i);
        lineObj.insert(QStringLiteral("ok"), res.ok);
        lineObj.insert(QStringLiteral("result"), res.result);
        if (!res.ok) {
            m_errorCount++;
            QJsonArray diags;
            QJsonObject d;
            d.insert(QStringLiteral("message"), res.error);
            diags.append(d);
            lineObj.insert(QStringLiteral("diagnostics"), diags);
        } else if (!res.result.isEmpty()) {
            m_resultCount++;
        }

        if (i < highlightLines.size()) {
            lineObj.insert(QStringLiteral("highlightedHtml"), highlightLines.at(i).toObject().value(QStringLiteral("highlightedHtml")));
        }

        m_lines.append(lineObj);
    }

    endResetModel();
    emit linesChanged();
}

QString DocumentModel::runHighlightWorker(const QString &source)
{
    QProcess process;
    process.setProgram(QStringLiteral("node"));
    process.setArguments({QStringLiteral(NUMI_KDE_SOURCE_DIR) + QStringLiteral("/src/gui/evaluate-document.js")});
    process.start();
    if (!process.waitForStarted(1000)) {
        return QStringLiteral("{\"lines\":[]}");
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
