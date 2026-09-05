#include "documentmodel.h"
#include "engineclient.h"
#include "kwinrulemanager.h"

#include <KWindowSystem>
#include <KX11Extras>
#include <netwm.h>
#include <QClipboard>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QTextStream>
#include <QWindow>
#include <QDateTime>
#include <QSettings>
#include <QStandardPaths>
#include <QHash>
#include <QSaveFile>

#ifndef NUMI_KDE_APP_ID
#define NUMI_KDE_APP_ID "online.skorin.numi-kde"
#endif

namespace {
// Descriptions identify our KWin rules inside kwinrulesrc.
const QLatin1String kMainRuleDescription("Numi-KDE main window (managed by numi-kde)");
const QLatin1String kSettingsRuleDescription("Numi-KDE settings window (managed by numi-kde)");
// Written by numi-kde <= 0.1.80 in KWin 5 format; cleaned up on first start.
const QLatin1String kLegacyRuleDescription("Numi-KDE keep above (managed)");
const QLatin1String kMainWindowTitle("Numi-KDE");
const QLatin1String kSettingsWindowTitle("Numi-KDE Settings");
}

DocumentModel::DocumentModel(QObject *parent)
    : QAbstractListModel(parent)
{
    // libqalculate lives in a separate process (numi-kde-engine); EngineClient
    // restarts it when it crashes or stalls, so a bad expression can never
    // take the application down.
    m_engine = new EngineClient(this);
    connect(m_engine, &EngineClient::evaluated, this, &DocumentModel::onEvaluated);
    connect(m_engine, &EngineClient::ratesUpdated, this, [this]() {
        if (!m_source.trimmed().isEmpty() && m_source.trimmed() != QStringLiteral("/help"))
            evaluate();
    });
    connect(m_engine, &EngineClient::networkStatusChanged, this, &DocumentModel::networkStatusChanged);
    connect(m_engine, &EngineClient::engineRestarted, this, [this]() {
        emit engineRestartsChanged();
        // Whatever was on screen came from the old engine; recompute it.
        if (!m_source.trimmed().isEmpty() && m_source.trimmed() != QStringLiteral("/help"))
            evaluate();
    });
    QSettings settings("numi-kde", "numi-kde");
    m_history = settings.value("history").value<QVariantList>();
    m_decimalPlaces = qBound(0, settings.value("decimalPlaces", 3).toInt(), 10);
    m_defaultCurrency = settings.value("defaultCurrency", "USD").toString().toUpper();

    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(50); // 50ms debounce
    connect(m_debounceTimer, &QTimer::timeout, this, &DocumentModel::evaluate);

    m_engine->configure(m_decimalPlaces, m_defaultCurrency);
    m_engine->start();
}

DocumentModel::~DocumentModel() = default;

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
        ++m_evalGeneration;   // anything still in flight is obsolete
        emit sourceChanged();
        beginResetModel();
        m_lines.clear();
        LineResult line;
        line.ok = true;
        line.highlightedHtml = QStringLiteral("<span style=\"color:#6fc4e8\">/help</span>");
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
    places = qBound(0, places, 10);
    if (m_decimalPlaces == places) return;
    m_decimalPlaces = places;
    QSettings settings("numi-kde", "numi-kde");
    settings.setValue("decimalPlaces", m_decimalPlaces);
    emit decimalPlacesChanged();
    evaluate();
}

QString DocumentModel::defaultCurrency() const { return m_defaultCurrency; }

void DocumentModel::setDefaultCurrency(const QString &currency)
{
    const QString upper = currency.trimmed().toUpper();
    if (m_defaultCurrency == upper) return;
    m_defaultCurrency = upper.isEmpty() ? QStringLiteral("USD") : upper;
    QSettings settings("numi-kde", "numi-kde");
    settings.setValue("defaultCurrency", m_defaultCurrency);
    emit defaultCurrencyChanged();
    evaluate();
}

QString DocumentModel::autostartPath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    return dir + QStringLiteral("/autostart/" NUMI_KDE_APP_ID ".desktop");
}

QString DocumentModel::legacyAutostartPath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    return dir + QStringLiteral("/autostart/numi-kde.desktop");
}

bool DocumentModel::autostart() const
{
    return QFile::exists(autostartPath()) || QFile::exists(legacyAutostartPath());
}

int DocumentModel::networkStatus() const
{
    return m_engine->networkStatus();
}

int DocumentModel::engineRestarts() const
{
    return m_engine->restartCount();
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
    const QString path = autostartPath();
    if (enable) {
        QDir().mkpath(QFileInfo(path).absolutePath());
        QSaveFile file(path);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "[Desktop Entry]\n";
            out << "Type=Application\n";
            out << "Name=Numi-KDE\n";
            out << "Comment=Document calculator, starts hidden in the system tray\n";
            out << "Exec=numi-kde --hidden\n";
            out << "Icon=" NUMI_KDE_APP_ID "\n";
            out << "Terminal=false\n";
            out << "X-KDE-autostart-phase=2\n";
            out.flush();
            file.commit();
        }
    } else {
        QFile::remove(path);
    }
    QFile::remove(legacyAutostartPath());
    emit autostartChanged();
}

void DocumentModel::migrateLegacyState()
{
    // <= 0.1.80: autostart entry under the old name with an absolute Exec path.
    if (QFile::exists(legacyAutostartPath()) && !QFile::exists(autostartPath()))
        setAutostart(true);

    // <= 0.1.80: hand-written KWin rule in KWin 5 format that KWin 6 ignores.
    KWinRuleManager rules;
    if (rules.cleanupLegacy({QString(kLegacyRuleDescription)}))
        KWinRuleManager::reloadKWin();
}

int DocumentModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_lines.size();
}

QVariant DocumentModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_lines.size())
        return {};

    const LineResult &line = m_lines.at(index.row());
    switch (role) {
    case LineRole:            return index.row() + 1;
    case ResultRole:          return line.result;
    case OkRole:              return line.ok;
    case DiagnosticRole:      return line.ok ? QString() : (line.error.isEmpty() ? QStringLiteral("Error") : line.error);
    case HighlightedHtmlRole: return line.highlightedHtml;
    default:                  return {};
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
    copyText(m_lines.at(row).result);
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
    return m_engine->completion(prefix);
}

QStringList DocumentModel::getCompletions(const QString &lineContext)
{
    return m_engine->completions(lineContext);
}

QString DocumentModel::highlightExample(const QString &line) const
{
    return m_engine->highlight(line);
}

void DocumentModel::setKeepAbove(bool above)
{
    m_keepAbove = above;
    if (applyKWinRules(above))
        KWinRuleManager::reloadKWin();

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
    // Called from main.cpp BEFORE win->show(). On Wayland, KWin applies window
    // rules at surface map time, so the rules must exist before the window is
    // mapped. applyKWinRules() is a no-op when nothing changed, so this never
    // disturbs the position KWin has remembered for us.
    if (!KWindowSystem::isPlatformWayland())
        return;
    if (applyKWinRules(m_keepAbove))
        KWinRuleManager::reloadKWin();
}

// Ensures both of our KWin rules exist with the wanted keep-above state.
// Returns true when kwinrulesrc was modified (caller reloads KWin).
bool DocumentModel::applyKWinRules(bool keepAbove)
{
    KWinRuleManager rules;
    bool changed = false;

    KWinRuleManager::RuleSpec main;
    main.description = kMainRuleDescription;
    main.wmClass = QStringLiteral(NUMI_KDE_APP_ID);
    main.title = kMainWindowTitle;
    main.keepAbove = keepAbove;
    main.skipTaskbar = true;
    main.rememberPosition = false; // KWin never persists this for Wayland windows; the KWin script does it
    changed |= rules.ensureRule(main);

    KWinRuleManager::RuleSpec settings;
    settings.description = kSettingsRuleDescription;
    settings.wmClass = QStringLiteral(NUMI_KDE_APP_ID);
    settings.title = kSettingsWindowTitle;
    settings.keepAbove = keepAbove;
    settings.skipTaskbar = true;
    settings.rememberPosition = false;
    changed |= rules.ensureRule(settings);

    return changed;
}

void DocumentModel::evaluate()
{
    m_engine->configure(m_decimalPlaces, m_defaultCurrency);
    m_engine->evaluate(++m_evalGeneration, m_source);
}

void DocumentModel::onEvaluated(quint64 generation, const QList<LineResult> &qalcResults)
{
    if (generation != m_evalGeneration) return;   // superseded while in flight

    beginResetModel();
    m_lines = qalcResults;
    m_errorCount = 0;
    m_resultCount = 0;
    m_total = 0.0;
    m_hasTotal = false;
    QHash<QString, QPair<int, double>> totalsByKey;

    for (const LineResult &res : std::as_const(m_lines)) {
        if (!res.ok) {
            m_errorCount++;
        } else if (!res.result.isEmpty()) {
            m_resultCount++;
            if (res.hasNumericValue && !res.totalKey.isEmpty()) {
                auto entry = totalsByKey.value(res.totalKey, qMakePair(0, 0.0));
                entry.first += 1;
                entry.second += res.numericValue;
                totalsByKey.insert(res.totalKey, entry);
            }
        }
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
