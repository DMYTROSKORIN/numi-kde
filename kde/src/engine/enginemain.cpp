// numi-kde-engine — hosts libqalculate in its own process.
//
// The GUI talks to this process over stdin/stdout (see engineprotocol.h).
// If a calculation never returns, the GUI kills and respawns us; libqalculate's
// process-wide state can never take the application down with it.

#include "../engineprotocol.h"
#include "../qalcbridge.h"

#include <QCoreApplication>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QThread>
#include <QtConcurrent>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>

#ifndef NUMI_KDE_VERSION
#define NUMI_KDE_VERSION "0.0.0"
#endif

namespace {

// Blocking stdin reader; the main thread stays free to run calculations and
// answer completion requests while a document is being evaluated.
class StdinReader : public QThread
{
    Q_OBJECT
public:
    using QThread::QThread;
signals:
    void lineReceived(const QByteArray &line);
    void inputClosed();
protected:
    void run() override
    {
        std::string line;
        while (std::getline(std::cin, line))
            emit lineReceived(QByteArray::fromStdString(line));
        emit inputClosed();
    }
};

class EngineServer : public QObject
{
    Q_OBJECT
public:
    EngineServer()
    {
        connect(&m_bridge, &QalcBridge::ratesUpdated, this, [this]() {
            send({{QStringLiteral("ev"), QStringLiteral("ratesUpdated")}});
        });
        connect(&m_bridge, &QalcBridge::networkStatusChanged, this, [this]() {
            send({{QStringLiteral("ev"), QStringLiteral("networkStatus")},
                  {QStringLiteral("value"), static_cast<int>(m_bridge.networkStatus())}});
        });
        connect(&m_watcher, &QFutureWatcher<QList<LineResult>>::finished, this, &EngineServer::onEvaluationFinished);
        send({{QStringLiteral("ev"), QStringLiteral("ready")},
              {QStringLiteral("version"), QStringLiteral(NUMI_KDE_VERSION)},
              {QStringLiteral("networkStatus"), static_cast<int>(m_bridge.networkStatus())}});
    }

public slots:
    void handleLine(const QByteArray &raw)
    {
        QJsonParseError err;
        const QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
        if (!doc.isObject()) {
            send({{QStringLiteral("ev"), QStringLiteral("error")},
                  {QStringLiteral("message"), QStringLiteral("bad request: %1").arg(err.errorString())}});
            return;
        }
        const QJsonObject req = doc.object();
        const QString op = req.value(QStringLiteral("op")).toString();
        const qint64 id = req.value(QStringLiteral("id")).toVariant().toLongLong();

        if (op == QLatin1String("configure")) {
            if (req.contains(QStringLiteral("decimals")))
                m_bridge.setDecimalPlaces(req.value(QStringLiteral("decimals")).toInt());
            if (req.contains(QStringLiteral("currency")))
                m_bridge.setDefaultCurrency(req.value(QStringLiteral("currency")).toString());
            reply(id, {});
        } else if (op == QLatin1String("evaluate")) {
            Job job;
            job.id = id;
            job.source = req.value(QStringLiteral("source")).toString();
            for (const QJsonValue &v : req.value(QStringLiteral("skip")).toArray())
                job.skip.insert(v.toInt());
            if (m_running) {
                // A newer document supersedes the running one and anything queued.
                m_bridge.abortCalculation();
                if (m_pending)
                    reply(m_pending->id, {{QStringLiteral("cancelled"), true}});
                m_pending = job;
            } else {
                startJob(job);
            }
        } else if (op == QLatin1String("cancel")) {
            const qint64 target = req.value(QStringLiteral("target")).toVariant().toLongLong();
            if (m_running && m_running->id == target)
                m_bridge.abortCalculation();
            if (m_pending && m_pending->id == target) {
                reply(m_pending->id, {{QStringLiteral("cancelled"), true}});
                m_pending.reset();
            }
            reply(id, {});
        } else if (op == QLatin1String("completions")) {
            reply(id, {{QStringLiteral("items"),
                        QJsonArray::fromStringList(m_bridge.getCompletions(req.value(QStringLiteral("context")).toString()))}});
        } else if (op == QLatin1String("completion")) {
            reply(id, {{QStringLiteral("value"), m_bridge.getCompletion(req.value(QStringLiteral("prefix")).toString())}});
        } else if (op == QLatin1String("highlight")) {
            reply(id, {{QStringLiteral("html"), m_bridge.highlightLine(req.value(QStringLiteral("line")).toString())}});
        } else if (op == QLatin1String("ping")) {
            reply(id, {{QStringLiteral("version"), QStringLiteral(NUMI_KDE_VERSION)}});
        } else if (op == QLatin1String("_crash")) {
            // Test hook: lets the GUI-side tests exercise crash recovery.
            std::fflush(stdout);
            std::abort();
        } else {
            reply(id, {{QStringLiteral("error"), QStringLiteral("unknown op '%1'").arg(op)}});
        }
    }

    void inputClosed()
    {
        // The GUI went away (or killed us on purpose); leave promptly. The
        // engine is never destroyed: a force-stopped libqalculate thread would
        // block its destructor, and the OS reclaims everything anyway.
        std::fflush(stdout);
        std::_Exit(0);
    }

private:
    struct Job {
        qint64 id = 0;
        QString source;
        QSet<int> skip;
    };

    void startJob(const Job &job)
    {
        m_running = job;
        const qint64 id = job.id;
        auto progress = [this, id](int index) {
            // Called on the worker thread after every line; report from the main thread.
            QMetaObject::invokeMethod(this, [this, id, index]() {
                send({{QStringLiteral("ev"), QStringLiteral("progress")},
                      {QStringLiteral("id"), id},
                      {QStringLiteral("index"), index}});
            }, Qt::QueuedConnection);
        };
        const QString source = job.source;
        const QSet<int> skip = job.skip;
        m_watcher.setFuture(QtConcurrent::run([this, source, skip, progress]() {
            return m_bridge.evaluateDocument(source, progress, skip);
        }));
    }

    void onEvaluationFinished()
    {
        if (!m_running) return;
        const qint64 id = m_running->id;
        m_running.reset();
        reply(id, {{QStringLiteral("lines"), EngineProtocol::toJson(m_watcher.result())}});
        if (m_pending) {
            const Job next = *m_pending;
            m_pending.reset();
            startJob(next);
        }
    }

    void reply(qint64 id, QJsonObject fields)
    {
        fields.insert(QStringLiteral("id"), id);
        send(fields);
    }

    void send(const QJsonObject &message)
    {
        const QByteArray bytes = EngineProtocol::encode(message);
        std::fwrite(bytes.constData(), 1, size_t(bytes.size()), stdout);
        std::fflush(stdout);
    }

    QalcBridge m_bridge;
    QFutureWatcher<QList<LineResult>> m_watcher;
    std::optional<Job> m_running;
    std::optional<Job> m_pending;
};

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    // Same identity as the GUI so QSettings/cache paths (rates cache) match.
    app.setApplicationName(QStringLiteral("numi-kde"));
    app.setOrganizationName(QStringLiteral("numi-kde"));

    if (argc > 1 && qstrcmp(argv[1], "--version") == 0) {
        std::printf("numi-kde-engine %s\n", NUMI_KDE_VERSION);
        return 0;
    }

    EngineServer server;
    StdinReader reader;
    QObject::connect(&reader, &StdinReader::lineReceived, &server, &EngineServer::handleLine, Qt::QueuedConnection);
    QObject::connect(&reader, &StdinReader::inputClosed, &server, &EngineServer::inputClosed, Qt::QueuedConnection);
    reader.start();
    return app.exec();
}

#include "enginemain.moc"
