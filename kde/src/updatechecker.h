#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

class QNetworkAccessManager;

class UpdateChecker : public QObject
{
    Q_OBJECT

public:
    explicit UpdateChecker(QObject *parent = nullptr);

    // Fire-and-forget async check. Emits updateAvailable() if a newer release exists.
    void checkAsync();

    // Returns true if the last successful check was more than 24 hours ago.
    // The timestamp is updated in checkAsync() after a successful network response.
    bool shouldAutoCheck();

signals:
    void updateAvailable(const QString &latestVersion, const QUrl &releaseUrl);
    void checkFinished(bool updateFound);

private:
    QNetworkAccessManager *m_nam;
};
