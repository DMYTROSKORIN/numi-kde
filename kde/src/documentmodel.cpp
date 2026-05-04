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
#include <QHash>
#include <QtConcurrent>

#include <QSaveFile>
#include <utility>

DocumentModel::DocumentModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_qalc = new QalcBridge(this);
    connect(m_qalc, &QalcBridge::ratesUpdated, this, [this]() {
        if (!m_source.trimmed().isEmpty() && m_source.trimmed() != QStringLiteral("/help"))
            evaluate();
    });
    connect(m_qalc, &QalcBridge::networkStatusChanged, this, &DocumentModel::networkStatusChanged);
    QSettings settings("numi-kde", "numi-kde");
    m_history = settings.value("history").value<QVariantList>();
    m_decimalPlaces = settings.value("decimalPlaces", 3).toInt();

    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(50); // 50ms debounce
    connect(m_debounceTimer, &QTimer::timeout, this, &DocumentModel::evaluate);

    connect(&m_watcher, &QFutureWatcher<QList<LineResult>>::finished,
            this, &DocumentModel::onEvaluationFinished);
}

DocumentModel::~DocumentModel()
{
    m_watcher.cancel();
    m_watcher.waitForFinished();
}

QString DocumentModel::source() const
{
    return m_source;
}

void DocumentModel::setSource(const QString &source)
{
    // /help: show empty results; inline help text is rendered in QML.
    if (source.trimmed() == "/help") {
        if (m_source == source) return;
        m_source = source;
        m_debounceTimer->stop();
        m_watcher.cancel();
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
        m_hasTotal = false;
        endResetModel();
        emit linesChanged();
        return;
    }

    if (m_source == source) return;
    m_source = source;
    emit sourceChanged();
    m_debounceTimer->start();
}

int DocumentModel::errorCount() const { return m_errorCount; }
int DocumentModel::resultCount() const { return m_resultCount; }
bool DocumentModel::hasTotal() const { return m_hasTotal; }
QVariantList DocumentModel::history() const { return m_history; }
int DocumentModel::decimalPlaces() const { return m_decimalPlaces; }
double DocumentModel::total() const { return m_total; }

void DocumentModel::setDecimalPlaces(int places)
{
    if (m_decimalPlaces == places) return;
    m_decimalPlaces = places;
    QSettings settings("numi-kde", "numi-kde");
    settings.setValue("decimalPlaces", m_decimalPlaces);
    emit decimalPlacesChanged();
    evaluate();
}

bool DocumentModel::autostart() const
{
    QString path = QDir::homePath() + "/.config/autostart/numi-kde.desktop";
    return QFile::exists(path);
}

int DocumentModel::networkStatus() const
{
    return static_cast<int>(m_qalc->networkStatus());
}

bool DocumentModel::isWayland() const
{
    return KWindowSystem::isPlatformWayland();
}

QString DocumentModel::version() const
{
#ifdef NUMI_KDE_VERSION
    return QStringLiteral(NUMI_KDE_VERSION);
#else
    return QStringLiteral("unknown");
#endif
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
            out << "Exec=" << QCoreApplication::applicationFilePath() << " --hidden\n";
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

    QSettings settings("numi-kde", "numi-kde");
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
    QSettings settings("numi-kde", "numi-kde");
    settings.remove("history");
    emit historyChanged();
}

QString DocumentModel::completeWord(const QString &prefix)
{
    return m_qalc->getCompletion(prefix);
}

QString DocumentModel::highlightExample(const QString &line) const
{
    return m_qalc->highlightLine(line);
}

void DocumentModel::setKeepAbove(bool above)
{
    // Write kwinrulesrc on both X11 and Wayland; KWin remembers position on Wayland.
    if (!m_kwinRuleApplied || above != m_keepAbove) {
        setKWinKeepAboveRule(above);
        m_kwinRuleApplied = true;
        m_keepAbove = above;
    }

    for (QWindow *win : qApp->allWindows()) {
        if (!win->isVisible()) continue;
        if (KWindowSystem::isPlatformX11()) {
            if (above) {
                KX11Extras::setState(win->winId(), NET::KeepAbove | NET::SkipTaskbar | NET::SkipPager);
            } else {
                // Keep-above off, but always stay out of the taskbar and pager.
                KX11Extras::clearState(win->winId(), NET::KeepAbove);
                KX11Extras::setState(win->winId(), NET::SkipTaskbar | NET::SkipPager);
            }
        }
    }
}

void DocumentModel::prepareShow()
{
    // Called from main.cpp BEFORE win->show(). On Wayland, KWin applies
    // window rules at surface map time, so we must write the rule before the
    // window becomes visible.
    if (!KWindowSystem::isPlatformWayland())
        return;
    
    // If the rule is already applied, do NOT rewrite it here. Rewriting the
    // rule while KWin is managing it (e.g. for position memory) can cause
    // the position to be lost or reset to default (center) because we might
    // be reading the rule file before KWin has had a chance to save the
    // latest position to it.
    if (m_kwinRuleApplied)
        return;

    setKWinKeepAboveRule(m_keepAbove);
    m_kwinRuleApplied = true;
    reloadKWinRules();
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

    // 1. We keep all rules EXCEPT our managed one. If KWin already remembered
    //    a position for that managed rule, preserve it when rewriting.
    QString rememberedPosition;
    QVector<RuleSection> keptRules;
    keptRules.reserve(numericRules.size() + 1);
    for (const RuleSection &section : std::as_const(numericRules)) {
        bool isManagedRule = false;
        QString sectionPosition;
        for (const QString &line : section.lines) {
            if (line == QStringLiteral("Description=%1").arg(managedDescription)) {
                isManagedRule = true;
            } else if (line.startsWith(QStringLiteral("position="))) {
                sectionPosition = line;
            }
        }
        if (isManagedRule) {
            rememberedPosition = sectionPosition;
        } else {
            keptRules.append(section);
        }
    }

    // 2. Add the managed rule. Skip-taskbar and skip-pager are always forced;
    //    keep-above is toggled by the `enabled` parameter.
    QStringList ruleLines = {
        QStringLiteral("Description=%1").arg(managedDescription),
        QStringLiteral("skiptaskbar=true"),
        QStringLiteral("skiptaskbarrule=2"),  // Force
        QStringLiteral("skippager=true"),
        QStringLiteral("skippagerrule=2"),    // Force
        QStringLiteral("above=%1").arg(enabled ? "true" : "false"),
        QStringLiteral("aboverule=2"),        // Force
        QStringLiteral("keepabove=%1").arg(enabled ? "true" : "false"),
        QStringLiteral("keepaboverule=2"),    // Force
        QStringLiteral("wmclass=numi-kde"),
        QStringLiteral("wmclasscomplete=false"),
        QStringLiteral("wmclassmatch=2"),     // Substring Match
        QStringLiteral("types=4294967295"),   // All window types
    };
    if (!rememberedPosition.isEmpty()) {
        ruleLines << rememberedPosition;
    }
    ruleLines << QStringLiteral("positionrule=4");  // Remember
    keptRules.append(RuleSection{QString(), ruleLines});

    QDir().mkpath(configDir);
    QSaveFile output(path);
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

    output.commit();

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
    
    // Cancel previous evaluation if still running. 
    // QalcBridge's mutex will ensure the background thread finishes its current atomic evaluateDocument call.
    if (m_watcher.isRunning()) {
        m_watcher.cancel();
    }

    auto future = QtConcurrent::run([this, src = m_source]() {
        return m_qalc->evaluateDocument(src);
    });
    m_watcher.setFuture(future);
}

void DocumentModel::onEvaluationFinished()
{
    if (m_watcher.isCanceled()) return;

    const auto qalcResults = m_watcher.result();

    beginResetModel();
    m_lines = QJsonArray();
    m_errorCount = 0;
    m_resultCount = 0;
    m_total = 0.0;
    m_hasTotal = false;
    QHash<QString, QPair<int, double>> totalsByKey;

    for (int i = 0; i < qalcResults.size(); ++i) {
        QJsonObject lineObj;
        const auto &res = qalcResults.at(i);
        lineObj.insert(QStringLiteral("ok"), res.ok);
        lineObj.insert(QStringLiteral("result"), res.result);
        lineObj.insert(QStringLiteral("highlightedHtml"), res.highlightedHtml);
        if (res.hasNumericValue)
            lineObj.insert(QStringLiteral("numericValue"), res.numericValue);
        if (!res.totalKey.isEmpty())
            lineObj.insert(QStringLiteral("totalKey"), res.totalKey);

        if (!res.ok) {
            m_errorCount++;
            QJsonArray diags;
            QJsonObject d;
            d.insert(QStringLiteral("message"), res.error.isEmpty() ? QStringLiteral("Error") : res.error);
            diags.append(d);
            lineObj.insert(QStringLiteral("diagnostics"), diags);
        } else if (!res.result.isEmpty()) {
            m_resultCount++;
            if (res.hasNumericValue && !res.totalKey.isEmpty()) {
                auto entry = totalsByKey.value(res.totalKey, qMakePair(0, 0.0));
                entry.first += 1;
                entry.second += res.numericValue;
                totalsByKey.insert(res.totalKey, entry);
            }
        }

        m_lines.append(lineObj);
    }

    if (totalsByKey.size() == 1) {
        const auto entry = totalsByKey.cbegin().value();
        if (entry.first >= 2) {
            m_total = entry.second;
            m_hasTotal = true;
        }
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
