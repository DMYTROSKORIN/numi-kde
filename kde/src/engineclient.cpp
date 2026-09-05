#include "engineclient.h"
#include "engineprotocol.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>

#ifndef NUMI_KDE_LIBEXECDIR
#define NUMI_KDE_LIBEXECDIR "/usr/libexec"
#endif

namespace {
const QLatin1String kEngineName("numi-kde-engine");
constexpr int kSyncTimeoutMs = 400;
constexpr int kRestartDelayMs = 150;
}

EngineClient::EngineClient(QObject *parent)
    : QObject(parent)
{
    m_watchdog.setSingleShot(true);
    connect(&m_watchdog, &QTimer::timeout, this, &EngineClient::onWatchdog);
    m_sinceLastRestart.start();
}

EngineClient::~EngineClient()
{
    m_stopping = true;
    if (m_process) {
        m_process->closeWriteChannel();      // engine exits on EOF
        if (!m_process->waitForFinished(500))
            m_process->kill();
    }
}

QString EngineClient::enginePath()
{
    const QByteArray env = qgetenv("NUMI_KDE_ENGINE");
    if (!env.isEmpty() && QFile::exists(QString::fromLocal8Bit(env)))
        return QString::fromLocal8Bit(env);

    // Development builds keep the engine next to the GUI binary. Installed
    // builds keep it in <prefix>/libexec relative to <prefix>/bin — resolved
    // from the running binary so the RPM's /usr prefix wins over whatever
    // prefix the tree was configured with (CPack repackages under /usr).
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        appDir + QLatin1Char('/') + kEngineName,
        QDir::cleanPath(appDir + QStringLiteral("/../libexec/") + kEngineName),
        QDir::cleanPath(appDir + QStringLiteral("/../lib/numi-kde/") + kEngineName),
        QStringLiteral(NUMI_KDE_LIBEXECDIR "/") + kEngineName,
        QStringLiteral("/usr/libexec/") + kEngineName,
        QStringLiteral("/usr/lib/numi-kde/") + kEngineName,
    };
    for (const QString &candidate : candidates)
        if (QFile::exists(candidate))
            return candidate;
    return candidates.at(3);   // for the error message
}

bool EngineClient::start()
{
    if (isRunning()) return true;
    startProcess();
    return isRunning();
}

bool EngineClient::isRunning() const
{
    return m_process && m_process->state() == QProcess::Running;
}

void EngineClient::startProcess()
{
    if (m_process) {
        m_process->disconnect(this);
        m_process->deleteLater();
        m_process = nullptr;
    }
    m_buffer.clear();
    m_process = new QProcess(this);
    m_process->setProgram(enginePath());
    m_process->setProcessChannelMode(QProcess::ForwardedErrorChannel);   // engine stderr → our stderr
    connect(m_process, &QProcess::readyReadStandardOutput, this, &EngineClient::readAvailable);
    connect(m_process, &QProcess::finished, this, &EngineClient::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
            qWarning() << "numi-kde: cannot start calculation engine" << m_process->program();
    });
    m_process->start(QIODevice::ReadWrite | QIODevice::Text);
    if (!m_process->waitForStarted(3000))
        return;
    sendMessage({{QStringLiteral("op"), QStringLiteral("configure")},
                 {QStringLiteral("id"), qint64(m_nextId++)},
                 {QStringLiteral("decimals"), m_decimals},
                 {QStringLiteral("currency"), m_currency}});
}

void EngineClient::scheduleRestart()
{
    if (m_stopping) return;
    // Back off when the engine keeps dying: 150 ms, 300, 600 … capped at 5 s.
    int delay = kRestartDelayMs;
    if (m_sinceLastRestart.elapsed() < 10000)
        delay = qMin(5000, kRestartDelayMs << qMin(m_restarts, 6));
    ++m_restarts;
    m_sinceLastRestart.restart();
    QTimer::singleShot(delay, this, [this]() {
        if (m_stopping) return;
        startProcess();
        emit engineRestarted();
    });
}

void EngineClient::configure(int decimals, const QString &currency)
{
    m_decimals = decimals;
    m_currency = currency;
    if (!isRunning()) return;
    sendMessage({{QStringLiteral("op"), QStringLiteral("configure")},
                 {QStringLiteral("id"), qint64(m_nextId++)},
                 {QStringLiteral("decimals"), decimals},
                 {QStringLiteral("currency"), currency}});
}

QJsonObject EngineClient::evaluateRequest(qint64 id, const QString &source) const
{
    QJsonArray skip;
    const QStringList lines = source.split(QLatin1Char('\n'));
    for (int i = 0; i < lines.size(); ++i)
        if (m_poisonedLines.contains(lines.at(i).trimmed()))
            skip.append(i);
    return {{QStringLiteral("op"), QStringLiteral("evaluate")},
            {QStringLiteral("id"), id},
            {QStringLiteral("source"), source},
            {QStringLiteral("skip"), skip}};
}

void EngineClient::evaluate(quint64 generation, const QString &source)
{
    if (!isRunning() && !start()) {
        // No engine at all: report every line as unavailable rather than hanging.
        Job job;
        job.generation = generation;
        job.lines = source.split(QLatin1Char('\n'));
        QList<LineResult> lines;
        for (int i = 0; i < job.lines.size(); ++i) {
            LineResult r;
            r.ok = false;
            r.error = QStringLiteral("Calculation engine is not available");
            lines.append(r);
        }
        emit evaluated(generation, lines);
        return;
    }

    // Older documents are obsolete: tell the engine so it stops early.
    for (auto it = m_inflight.begin(); it != m_inflight.end(); ++it) {
        sendMessage({{QStringLiteral("op"), QStringLiteral("cancel")},
                     {QStringLiteral("id"), qint64(m_nextId++)},
                     {QStringLiteral("target"), it.key()}});
    }

    Job job;
    job.generation = generation;
    job.lines = source.split(QLatin1Char('\n'));
    // Request ids come from the same counter as every other message, so a
    // configure/completion reply can never be mistaken for an evaluate reply.
    const qint64 id = m_nextId++;
    m_inflight.insert(id, job);
    sendMessage(evaluateRequest(id, source));
    m_watchdog.start(m_watchdogMs);
}

void EngineClient::sendMessage(const QJsonObject &message)
{
    if (!isRunning()) return;
    m_process->write(EngineProtocol::encode(message));
}

void EngineClient::readAvailable()
{
    if (!m_process) return;
    m_buffer += m_process->readAllStandardOutput();
    int nl;
    while ((nl = m_buffer.indexOf('\n')) >= 0) {
        const QByteArray line = m_buffer.left(nl);
        m_buffer.remove(0, nl + 1);
        if (line.trimmed().isEmpty()) continue;
        const QJsonDocument doc = QJsonDocument::fromJson(line);
        if (doc.isObject())
            dispatch(doc.object());
        else
            qWarning() << "numi-kde: engine sent malformed line:" << line.left(200);
    }
}

void EngineClient::dispatch(const QJsonObject &m)
{
    const QString ev = m.value(QStringLiteral("ev")).toString();
    if (!ev.isEmpty()) {
        if (ev == QLatin1String("progress")) {
            const qint64 id = m.value(QStringLiteral("id")).toVariant().toLongLong();
            auto it = m_inflight.find(id);
            if (it != m_inflight.end()) {
                it->progressed = m.value(QStringLiteral("index")).toInt();
                m_watchdog.start(m_watchdogMs);   // the engine is alive and working
            }
        } else if (ev == QLatin1String("ratesUpdated")) {
            emit ratesUpdated();
        } else if (ev == QLatin1String("networkStatus")) {
            m_networkStatus = m.value(QStringLiteral("value")).toInt();
            emit networkStatusChanged();
        } else if (ev == QLatin1String("ready")) {
            m_engineVersion = m.value(QStringLiteral("version")).toString();
            m_networkStatus = m.value(QStringLiteral("networkStatus")).toInt(m_networkStatus);
            emit networkStatusChanged();
        } else if (ev == QLatin1String("error")) {
            qWarning() << "numi-kde: engine:" << m.value(QStringLiteral("message")).toString();
        }
        return;
    }

    const qint64 id = m.value(QStringLiteral("id")).toVariant().toLongLong();
    auto it = m_inflight.find(id);
    if (it != m_inflight.end()) {
        const Job job = it.value();
        m_inflight.erase(it);
        if (m_inflight.isEmpty())
            m_watchdog.stop();
        if (m.contains(QStringLiteral("lines")))
            emit evaluated(job.generation, EngineProtocol::linesFromJson(m.value(QStringLiteral("lines")).toArray()));
        // cancelled: superseded on purpose, nothing to report
        return;
    }
    m_syncReplies.insert(id, m);   // consumed by waitForReply()
}

std::optional<QJsonObject> EngineClient::waitForReply(qint64 id, int timeoutMs)
{
    QElapsedTimer t;
    t.start();
    while (isRunning()) {
        auto it = m_syncReplies.find(id);
        if (it != m_syncReplies.end()) {
            const QJsonObject reply = it.value();
            m_syncReplies.erase(it);
            return reply;
        }
        const int remaining = timeoutMs - int(t.elapsed());
        if (remaining <= 0) break;
        if (m_process->waitForReadyRead(remaining))
            readAvailable();
    }
    m_syncReplies.remove(id);
    return std::nullopt;
}

QStringList EngineClient::completions(const QString &context)
{
    if (!isRunning() && !start()) return {};
    const qint64 id = m_nextId++;
    sendMessage({{QStringLiteral("op"), QStringLiteral("completions")}, {QStringLiteral("id"), id}, {QStringLiteral("context"), context}});
    const auto reply = waitForReply(id, kSyncTimeoutMs);
    if (!reply) return {};
    QStringList items;
    for (const QJsonValue &v : reply->value(QStringLiteral("items")).toArray()) items << v.toString();
    return items;
}

QString EngineClient::completion(const QString &prefix)
{
    if (!isRunning() && !start()) return prefix;
    const qint64 id = m_nextId++;
    sendMessage({{QStringLiteral("op"), QStringLiteral("completion")}, {QStringLiteral("id"), id}, {QStringLiteral("prefix"), prefix}});
    const auto reply = waitForReply(id, kSyncTimeoutMs);
    return reply ? reply->value(QStringLiteral("value")).toString() : prefix;
}

QString EngineClient::highlight(const QString &line)
{
    if (!isRunning() && !start()) return line.toHtmlEscaped();
    const qint64 id = m_nextId++;
    sendMessage({{QStringLiteral("op"), QStringLiteral("highlight")}, {QStringLiteral("id"), id}, {QStringLiteral("line"), line}});
    const auto reply = waitForReply(id, kSyncTimeoutMs);
    return reply ? reply->value(QStringLiteral("html")).toString() : line.toHtmlEscaped();
}

void EngineClient::debugCrashEngine()
{
    sendMessage({{QStringLiteral("op"), QStringLiteral("_crash")}, {QStringLiteral("id"), qint64(m_nextId++)}});
}

QList<LineResult> EngineClient::partialResults(const Job &job, bool poisonCurrent)
{
    QList<LineResult> lines;
    const int current = job.progressed + 1;
    for (int i = 0; i < job.lines.size(); ++i) {
        LineResult r;
        if (i == current) {
            r.ok = false;
            r.error = QStringLiteral("Calculation took too long");
            if (poisonCurrent)
                m_poisonedLines.insert(job.lines.at(i).trimmed());
        } else if (i < current) {
            // Finished before the engine died; the actual value is gone with the
            // process, so show nothing rather than something wrong. The next
            // evaluation (with the poisoned line skipped) fills it in.
            r.ok = true;
        } else {
            r.ok = true;
        }
        lines.append(r);
    }
    return lines;
}

void EngineClient::onWatchdog()
{
    if (m_inflight.isEmpty() || !m_process) return;
    // The engine stopped making progress: the newest job is what the user
    // sees; poison the line it is stuck on and start over.
    qint64 newestId = 0;
    for (auto it = m_inflight.cbegin(); it != m_inflight.cend(); ++it)
        if (it.key() > newestId) newestId = it.key();
    const Job newest = m_inflight.value(newestId);
    m_inflight.clear();
    qWarning() << "numi-kde: calculation engine stalled on line" << (newest.progressed + 2) << "- restarting it";

    m_process->disconnect(this);
    m_process->kill();
    m_process->waitForFinished(1000);
    m_process->deleteLater();
    m_process = nullptr;

    const QList<LineResult> partial = partialResults(newest, true);
    scheduleRestart();
    emit evaluated(newest.generation, partial);
}

void EngineClient::onProcessFinished(int exitCode, int exitStatus)
{
    if (m_stopping) return;
    const bool crashed = exitStatus == QProcess::CrashExit || exitCode != 0;
    if (crashed)
        qWarning() << "numi-kde: calculation engine exited unexpectedly (code" << exitCode << ") - restarting it";
    m_watchdog.stop();
    // Whatever was in flight died with the process; report it and move on.
    QHash<qint64, Job> inflight;
    inflight.swap(m_inflight);
    m_process->deleteLater();
    m_process = nullptr;
    scheduleRestart();
    for (auto it = inflight.cbegin(); it != inflight.cend(); ++it)
        emit evaluated(it->generation, partialResults(it.value(), crashed));
}
