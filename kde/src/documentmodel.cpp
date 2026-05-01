#include "documentmodel.h"
#include "qalcbridge.h"

#include <KWindowSystem>
#include <KX11Extras>
#include <netwm.h>
#include <QClipboard>
#include <QCoreApplication>
#include <QDBusInterface>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QTextStream>
#include <QWindow>
#include <QDateTime>
#include <QSettings>
#include <QStandardPaths>
#include <QVector>

#include <utility>

DocumentModel::DocumentModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_qalc = new QalcBridge(this);
    QSettings settings("skorin", "numi-kde");
    m_history = settings.value("history").value<QVariantList>();
    m_decimalPlaces = settings.value("decimalPlaces", 3).toInt();
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
    // /help: show empty results, help is handled by QML overlay
    if (source.trimmed() == "/help") {
        if (m_source == source) return;
        m_source = source;
        emit sourceChanged();
        beginResetModel();
        m_lines = QJsonArray();
        QJsonObject line;
        line.insert(QStringLiteral("ok"), true);
        line.insert(QStringLiteral("result"), QString()); // Empty result
        line.insert(QStringLiteral("highlightedHtml"),
                    QStringLiteral("<span style=\"color:#6fc4e8\">/help</span>"));
        m_lines.append(line);
        m_errorCount = 0;
        m_resultCount = 0; // Don't count /help as a result
        m_total = 0.0;
        endResetModel();
        emit linesChanged();
        return;
    }

    if (m_source == source) return;
    m_source = source;
    emit sourceChanged();
    evaluate();
}

int DocumentModel::errorCount() const { return m_errorCount; }
int DocumentModel::resultCount() const { return m_resultCount; }
QVariantList DocumentModel::history() const { return m_history; }
int DocumentModel::decimalPlaces() const { return m_decimalPlaces; }
double DocumentModel::total() const { return m_total; }

void DocumentModel::setDecimalPlaces(int places)
{
    if (m_decimalPlaces == places) return;
    m_decimalPlaces = places;
    QSettings settings("skorin", "numi-kde");
    settings.setValue("decimalPlaces", m_decimalPlaces);
    emit decimalPlacesChanged();
    evaluate();
}

bool DocumentModel::autostart() const
{
    QString path = QDir::homePath() + "/.config/autostart/numi-kde.desktop";
    return QFile::exists(path);
}

void DocumentModel::setAutostart(bool enable)
{
    QString path = QDir::homePath() + "/.config/autostart/numi-kde.desktop";
    if (enable) {
        QDir().mkpath(QDir::homePath() + "/.config/autostart");
        QFile file(path);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "[Desktop Entry]\n";
            out << "Type=Application\n";
            out << "Name=Numi-KDE\n";
            out << "Exec=" << QCoreApplication::applicationFilePath() << "\n";
            out << "Hidden=false\n";
            out << "NoDisplay=false\n";
            out << "X-GNOME-Autostart-enabled=true\n";
        }
    } else {
        QFile::remove(path);
    }
    emit autostartChanged();
}

int DocumentModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_lines.size();
}

QVariant DocumentModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_lines.size())
        return {};

    const auto line = m_lines.at(index.row()).toObject();
    switch (role) {
    case LineRole:          return line.value(QStringLiteral("number")).toInt();
    case ResultRole:        return line.value(QStringLiteral("result")).toString();
    case OkRole:            return line.value(QStringLiteral("ok")).toBool();
    case DiagnosticRole:    return firstDiagnostic(line);
    case HighlightedHtmlRole: return line.value(QStringLiteral("highlightedHtml")).toString();
    default:                return {};
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
    if (row < 0 || row >= m_lines.size()) return;
    const auto result = m_lines.at(row).toObject().value(QStringLiteral("result")).toString();
    copyText(result);
}

void DocumentModel::copyText(const QString &text)
{
    if (!text.isEmpty())
        QGuiApplication::clipboard()->setText(text);
}

void DocumentModel::saveSession()
{
    if (m_source.trimmed().isEmpty() || m_source.trimmed() == "/help") return;

    if (!m_history.isEmpty()) {
        const auto last = m_history.first().toMap();
        if (last.value("text").toString() == m_source) return;
    }

    QVariantMap entry;
    entry["text"] = m_source;
    entry["timestamp"] = QDateTime::currentDateTime().toString("MMM d, hh:mm");

    m_history.prepend(entry);
    if (m_history.size() > 25)
        m_history.removeLast();

    QSettings settings("skorin", "numi-kde");
    settings.setValue("history", m_history);
    emit historyChanged();
}

void DocumentModel::restoreSession(int index)
{
    if (index < 0 || index >= m_history.size()) return;
    const auto entry = m_history.at(index).toMap();
    setSource(entry.value("text").toString());
}

void DocumentModel::clearHistory()
{
    m_history.clear();
    QSettings settings("skorin", "numi-kde");
    settings.remove("history");
    emit historyChanged();
}

QString DocumentModel::completeWord(const QString &prefix)
{
    return m_qalc->getCompletion(prefix);
}

void DocumentModel::setKeepAbove(bool above)
{
    if (KWindowSystem::isPlatformWayland())
        setKWinKeepAboveRule(above);

    for (QWindow *win : qApp->allWindows()) {
        if (!win->isVisible()) continue;
        if (KWindowSystem::isPlatformX11()) {
            if (above) {
                KX11Extras::setState(win->winId(), NET::KeepAbove | NET::SkipTaskbar);
            } else {
                KX11Extras::clearState(win->winId(), NET::KeepAbove | NET::SkipTaskbar);
            }
        }
    }
}

void DocumentModel::setKWinKeepAboveRule(bool enabled)
{
    const QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    if (configDir.isEmpty())
        return;

    const QString path = configDir + QStringLiteral("/kwinrulesrc");
    const QString managedDescription = QStringLiteral("Numi-KDE keep above (managed)");

    struct RuleSection {
        QString name;
        QStringList lines;
    };

    QVector<RuleSection> numericRules;
    QVector<RuleSection> otherSections;

    QFile input(path);
    if (input.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&input);
        RuleSection current;
        while (!in.atEnd()) {
            const QString line = in.readLine();
            if (line.startsWith('[') && line.endsWith(']')) {
                if (!current.name.isEmpty()) {
                    bool isNumber = false;
                    current.name.toInt(&isNumber);
                    if (isNumber)
                        numericRules.append(current);
                    else if (current.name != QStringLiteral("General") &&
                             current.name != QStringLiteral("%General"))
                        otherSections.append(current);
                }
                current = RuleSection{line.mid(1, line.size() - 2), {}};
            } else if (!current.name.isEmpty()) {
                current.lines.append(line);
            }
        }
        if (!current.name.isEmpty()) {
            bool isNumber = false;
            current.name.toInt(&isNumber);
            if (isNumber)
                numericRules.append(current);
            else if (current.name != QStringLiteral("General") &&
                     current.name != QStringLiteral("%General"))
                otherSections.append(current);
        }
        input.close();
    }

    // 1. We keep all rules EXCEPT our managed one.
    QVector<RuleSection> keptRules;
    keptRules.reserve(numericRules.size() + 1);
    for (const RuleSection &section : std::as_const(numericRules)) {
        bool isManagedRule = false;
        for (const QString &line : section.lines) {
            if (line == QStringLiteral("Description=%1").arg(managedDescription)) {
                isManagedRule = true;
                break;
            }
        }
        if (!isManagedRule)
            keptRules.append(section);
    }

    // 2. If enabled, add the Keep Above rule.
    if (enabled) {
        keptRules.append(RuleSection{
            QString(),
            {
                QStringLiteral("Description=%1").arg(managedDescription),
                QStringLiteral("above=true"),
                QStringLiteral("aboverule=2"), // 2 = Force
                QStringLiteral("keepabove=true"),
                QStringLiteral("keepaboverule=2"), // 2 = Force
                QStringLiteral("wmclass=numi-kde"),
                QStringLiteral("wmclasscomplete=false"),
                QStringLiteral("wmclassmatch=2"), // 2 = Exact Match Substring
                QStringLiteral("types=4294967295"), // All window types
            }
        });
    } else {
        // To ensure it gets turned off instantly, we append a temporary rule
        // forcing it off, then rely on DBus reconfigure. KWin caches window states.
        // Or simply omitting it is usually enough for KWin to drop the state on reconfigure
        // but if it sticks, it means the state needs to be forcefully cleared.
        // Let's add a force-disable rule if not enabled to ensure the state clears.
        keptRules.append(RuleSection{
            QString(),
            {
                QStringLiteral("Description=%1").arg(managedDescription),
                QStringLiteral("above=false"),
                QStringLiteral("aboverule=2"), // Force
                QStringLiteral("keepabove=false"),
                QStringLiteral("keepaboverule=2"), // Force
                QStringLiteral("wmclass=numi-kde"),
                QStringLiteral("wmclasscomplete=false"),
                QStringLiteral("wmclassmatch=2"),
                QStringLiteral("types=4294967295"),
            }
        });
    }

    QDir().mkpath(configDir);
    QFile output(path);
    if (!output.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&output);
    out << "[General]\n";
    out << "count=" << keptRules.size() << "\n\n";

    for (int i = 0; i < keptRules.size(); ++i) {
        out << "[" << (i + 1) << "]\n";
        for (const QString &line : std::as_const(keptRules[i].lines)) {
            if (!line.isEmpty())
                out << line << "\n";
        }
        out << "\n";
    }

    for (const RuleSection &section : std::as_const(otherSections)) {
        out << "[" << section.name << "]\n";
        for (const QString &line : section.lines)
            out << line << "\n";
        out << "\n";
    }

    output.close();

    reloadKWinRules();
}

void DocumentModel::reloadKWinRules()
{
    QDBusInterface kwin(QStringLiteral("org.kde.KWin"),
                        QStringLiteral("/KWin"),
                        QStringLiteral("org.kde.KWin"));
    if (kwin.isValid()) {
        kwin.call(QStringLiteral("reconfigure"));
    }
}

void DocumentModel::evaluate()
{
    m_qalc->setDecimalPlaces(m_decimalPlaces);
    const auto qalcResults = m_qalc->evaluateDocument(m_source);

    beginResetModel();
    m_lines = QJsonArray();
    m_errorCount = 0;
    m_resultCount = 0;
    m_total = 0.0;

    for (int i = 0; i < qalcResults.size(); ++i) {
        QJsonObject lineObj;
        const auto &res = qalcResults.at(i);
        lineObj.insert(QStringLiteral("ok"), res.ok);
        lineObj.insert(QStringLiteral("result"), res.result);
        lineObj.insert(QStringLiteral("highlightedHtml"), res.highlightedHtml);
        if (res.hasNumericValue)
            lineObj.insert(QStringLiteral("numericValue"), res.numericValue);

        if (!res.ok) {
            m_errorCount++;
            QJsonArray diags;
            QJsonObject d;
            d.insert(QStringLiteral("message"), res.error.isEmpty() ? QStringLiteral("Error") : res.error);
            diags.append(d);
            lineObj.insert(QStringLiteral("diagnostics"), diags);
        } else if (!res.result.isEmpty()) {
            m_resultCount++;
            if (res.hasNumericValue)
                m_total += res.numericValue;
        }

        m_lines.append(lineObj);
    }

    endResetModel();
    emit linesChanged();
}

QString DocumentModel::firstDiagnostic(const QJsonObject &line)
{
    const auto diagnostics = line.value(QStringLiteral("diagnostics")).toArray();
    if (diagnostics.isEmpty()) return {};
    return diagnostics.first().toObject().value(QStringLiteral("message")).toString();
}
