#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QUrl>

class QNetworkAccessManager;
class QProcess;

/**
 * Self-update flow:
 *   check (GitHub releases/latest) → UpdateAvailable → install → RestartRequired.
 *
 * Installation is delegated to the privileged helper
 * /usr/libexec/numi-kde-install-update, which receives only the version
 * number and downloads + verifies the RPM itself (see the helper script).
 * Nothing is downloaded into the user's session anymore.
 */
class UpdateChecker : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool autoInstallUpdates READ autoInstallUpdates
               WRITE setAutoInstallUpdates NOTIFY autoInstallUpdatesChanged)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString availableVersion READ availableVersion NOTIFY availableVersionChanged)

public:
    enum class State {
        Idle,
        Checking,
        UpdateAvailable,   // newer version found, not installing yet
        Installing,        // pkexec + helper running
        RestartRequired,   // install complete, new binary on disk
        Error              // last install attempt failed; retry allowed
    };
    Q_ENUM(State)

    explicit UpdateChecker(QObject *parent = nullptr);

    bool    autoInstallUpdates() const { return m_autoInstall; }
    State   state()              const { return m_state; }
    QString availableVersion()   const { return m_availableVersion; }
    QString lastError()          const { return m_lastError; }

    /// True if the last successful check was more than 24 hours ago.
    bool shouldAutoCheck();

    /// Version the app ran as last time (empty on first start). Updates the
    /// stored value to the current version; returns the previous one.
    static QString consumeLastRunVersion();

public slots:
    void checkAsync();
    void installUpdate();
    void restartApp();
    void setAutoInstallUpdates(bool enabled);

signals:
    void autoInstallUpdatesChanged();
    void stateChanged(UpdateChecker::State state);
    void availableVersionChanged();

    // Convenience signals for main.cpp / tray
    void updateAvailable(const QString &version, const QUrl &releaseUrl);
    void installFinished(bool success);
    void checkFinished(bool updateFound);

private:
    void setState(State s);

    QNetworkAccessManager *m_nam           = nullptr;
    QProcess              *m_installer     = nullptr;
    QTimer                *m_periodicTimer = nullptr;

    State   m_state       = State::Idle;
    bool    m_autoInstall = true;
    QString m_availableVersion;
    QString m_lastError;
};
