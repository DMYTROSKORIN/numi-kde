#include "updatechecker.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QUrl>

#ifndef NUMI_KDE_VERSION
#define NUMI_KDE_VERSION "0.0.0"
#endif

static const char kApiUrl[] =
    "https://api.github.com/repos/DMYTROSKORIN/numi-kde/releases/latest";

static bool isNewerVersion(const QString &latest, const QString &current)
{
    auto parse = [](QString v) -> QList<int> {
        if (v.startsWith(QLatin1Char('v')))
            v = v.mid(1);
        QList<int> parts;
        for (const QString &p : v.split(QLatin1Char('.')))
            parts << p.toInt();
        while (parts.size() < 3)
            parts << 0;
        return parts;
    };
    const auto l = parse(latest);
    const auto c = parse(current);
    for (int i = 0; i < 3; ++i) {
        if (l[i] > c[i]) return true;
        if (l[i] < c[i]) return false;
    }
    return false;
}

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent), m_nam(new QNetworkAccessManager(this))
{}

bool UpdateChecker::shouldAutoCheck()
{
    QSettings s(QStringLiteral("numi-kde"), QStringLiteral("numi-kde"));
    s.beginGroup(QStringLiteral("Updates"));
    const QDateTime last = s.value(QStringLiteral("lastCheck")).toDateTime();
    const bool due = !last.isValid() || last.secsTo(QDateTime::currentDateTimeUtc()) >= 86400;
    if (due)
        s.setValue(QStringLiteral("lastCheck"), QDateTime::currentDateTimeUtc());
    s.endGroup();
    return due;
}

void UpdateChecker::checkAsync()
{
    QNetworkRequest req(QUrl(QString::fromLatin1(kApiUrl)));
    req.setTransferTimeout(10000);
    req.setRawHeader("Accept", "application/vnd.github+json");
    req.setRawHeader("User-Agent", "numi-kde/" NUMI_KDE_VERSION);

    QNetworkReply *reply = m_nam->get(req);
    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit checkFinished(false);
            return;
        }
        const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        const QString tag = obj.value(QStringLiteral("tag_name")).toString();
        const QString url = obj.value(QStringLiteral("html_url")).toString();
        const bool found = !tag.isEmpty() && isNewerVersion(tag, QStringLiteral(NUMI_KDE_VERSION));
        if (found && url.startsWith(QStringLiteral("https://github.com/DMYTROSKORIN/numi-kde")))
            emit updateAvailable(tag, QUrl(url));
        emit checkFinished(found);
    });
}
