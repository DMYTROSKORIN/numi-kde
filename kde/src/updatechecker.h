#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QUrl>

class QFile;
class QNetworkAccessManager;
namespace PackageKit { class Transaction; }

class UpdateChecker : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool autoDownloadUpdates READ autoDownloadUpdates
               WRITE setAutoDownloadUpdates NOTIFY autoDownloadUpdatesChanged)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(int  downloadProgress READ downloadProgress NOTIFY downloadProgressChanged)
    Q_PROPERTY(QString availableVersion READ availableVersion NOTIFY availableVersionChanged)

public:
    enum class State {
        Idle,
        Checking,
        UpdateAvailable,   // newer version found, not yet downloading
        Downloading,       // RPM download in progress
        DownloadReady,     // RPM on disk, ready for PackageKit
        Installing,        // PackageKit transaction running (polkit dialog may be open)
        RestartRequired,   // install complete
        Error
    };
    Q_ENUM(State)

    explicit UpdateChecker(QObject *parent = nullptr);
    ~UpdateChecker() override;

    bool    autoDownloadUpdates() const { return m_autoDownload; }
    State   state()               const { return m_state; }
    int     downloadProgress()    const { return m_downloadProgress; }
    QString availableVersion()    const { return m_availableVersion; }
    QString lastError()           const { return m_lastError; }

    // Returns true if the last successful check was more than 24 hours ago.
    bool shouldAutoCheck();

public slots:
    void checkAsync();
    void startDownload();
    void installUpdate();
    void restartApp();
    void setAutoDownloadUpdates(bool enabled);

signals:
    void autoDownloadUpdatesChanged();
    void stateChanged(UpdateChecker::State state);
    void downloadProgressChanged(int progress);
    void availableVersionChanged();

    // Convenience signals for main.cpp / tray
    void updateAvailable(const QString &version, const QUrl &releaseUrl);
    void downloadReady(const QString &version);
    void installFinished(bool success);
    void checkFinished(bool updateFound);

private:
    void setState(State s);
    void setDownloadProgress(int p);

    QNetworkAccessManager     *m_nam             = nullptr;
    QFile                     *m_dlFile          = nullptr;
    PackageKit::Transaction   *m_pkTransaction   = nullptr;
    QTimer                    *m_periodicTimer   = nullptr;

    State   m_state             = State::Idle;
    int     m_downloadProgress  = 0;
    bool    m_autoDownload      = true;
    QString m_availableVersion;
    QString m_lastError;
    QUrl    m_downloadUrl;
    QString m_rpmPath;
};
