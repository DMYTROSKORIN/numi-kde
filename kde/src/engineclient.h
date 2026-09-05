#pragma once

#include "lineresult.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTimer>

#include <optional>

class QProcess;

/**
 * GUI-side owner of the numi-kde-engine process (see engineprotocol.h).
 *
 * Reliability model: libqalculate runs in the child. If the child crashes or
 * stops making progress (no finished line for `watchdogMs`), it is killed and
 * respawned, the line it was working on is *poisoned* (reported as
 * "Calculation took too long" and never sent again while its text is
 * unchanged), and the document result is delivered with what was computed.
 */
class EngineClient : public QObject
{
    Q_OBJECT
public:
    explicit EngineClient(QObject *parent = nullptr);
    ~EngineClient() override;

    /// Path of the engine binary: $NUMI_KDE_ENGINE, next to the GUI binary
    /// (development builds), or the installed libexec location.
    static QString enginePath();

    bool start();
    bool isRunning() const;
    int restartCount() const { return m_restarts; }
    void setWatchdogMs(int ms) { m_watchdogMs = ms; }

    /// Remembered per session; applied to every evaluate and re-sent after a restart.
    void configure(int decimals, const QString &currency);

    /// Asynchronous; the answer arrives as evaluated(generation, lines).
    /// A newer generation supersedes the older one on the engine side.
    void evaluate(quint64 generation, const QString &source);

    /// Synchronous helpers with a short timeout; empty results when the engine
    /// is not available (the GUI must never block on libqalculate).
    QStringList completions(const QString &context);
    QString completion(const QString &prefix);
    QString highlight(const QString &line);

    int networkStatus() const { return m_networkStatus; }
    QString engineVersion() const { return m_engineVersion; }

    /// Test hook: asks the engine to abort() itself so crash recovery can be exercised.
    void debugCrashEngine();

signals:
    void evaluated(quint64 generation, const QList<LineResult> &lines);
    void ratesUpdated();
    void networkStatusChanged();
    void engineRestarted();

private:
    struct Job {
        quint64 generation = 0;
        QStringList lines;      // source split by '\n', for partial results
        int progressed = -1;    // index of the last finished line
    };

    void startProcess();
    void scheduleRestart();
    void sendMessage(const QJsonObject &message);
    void readAvailable();
    void dispatch(const QJsonObject &message);
    std::optional<QJsonObject> waitForReply(qint64 id, int timeoutMs);
    void onWatchdog();
    void onProcessFinished(int exitCode, int exitStatus);
    QList<LineResult> partialResults(const Job &job, bool poisonCurrent);
    QJsonObject evaluateRequest(qint64 id, const QString &source) const;

    QProcess *m_process = nullptr;
    QByteArray m_buffer;
    qint64 m_nextId = 1;
    QHash<qint64, Job> m_inflight;       // request id → job (generation inside)
    QHash<qint64, QJsonObject> m_syncReplies;
    QSet<QString> m_poisonedLines;
    QTimer m_watchdog;
    int m_watchdogMs = 6000;
    int m_restarts = 0;
    QElapsedTimer m_sinceLastRestart;
    bool m_stopping = false;
    int m_decimals = 3;
    QString m_currency = QStringLiteral("USD");
    int m_networkStatus = 0;
    QString m_engineVersion;
};
