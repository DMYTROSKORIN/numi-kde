#include "updatechecker.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QSysInfo>
#include <QUrl>

#include <PackageKit/Daemon>
#include <PackageKit/Transaction>

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
{
    QSettings s(QStringLiteral("numi-kde"), QStringLiteral("numi-kde"));
    s.beginGroup(QStringLiteral("Updates"));
    m_autoDownload = s.value(QStringLiteral("autoDownload"), true).toBool();
    s.endGroup();

    // Check once per hour; the actual 24 h gate is inside shouldAutoCheck().
    m_periodicTimer = new QTimer(this);
    m_periodicTimer->setInterval(60 * 60 * 1000);
    m_periodicTimer->setSingleShot(false);
    QObject::connect(m_periodicTimer, &QTimer::timeout, this, [this]() {
        if (shouldAutoCheck())
            checkAsync();
    });
    m_periodicTimer->start();
}

UpdateChecker::~UpdateChecker()
{
    if (m_dlFile) {
        m_dlFile->close();
        delete m_dlFile;
    }
}

bool UpdateChecker::shouldAutoCheck()
{
    QSettings s(QStringLiteral("numi-kde"), QStringLiteral("numi-kde"));
    s.beginGroup(QStringLiteral("Updates"));
    const QDateTime last = s.value(QStringLiteral("lastCheck")).toDateTime();
    s.endGroup();
    return !last.isValid() || last.secsTo(QDateTime::currentDateTimeUtc()) >= 86400;
}

void UpdateChecker::setAutoDownloadUpdates(bool enabled)
{
    if (m_autoDownload == enabled)
        return;
    m_autoDownload = enabled;
    QSettings s(QStringLiteral("numi-kde"), QStringLiteral("numi-kde"));
    s.beginGroup(QStringLiteral("Updates"));
    s.setValue(QStringLiteral("autoDownload"), enabled);
    s.endGroup();
    emit autoDownloadUpdatesChanged();
}

void UpdateChecker::setState(State s)
{
    if (m_state == s)
        return;
    m_state = s;
    emit stateChanged(s);
}

void UpdateChecker::setDownloadProgress(int p)
{
    if (m_downloadProgress == p)
        return;
    m_downloadProgress = p;
    emit downloadProgressChanged(p);
}

// ── Version check ────────────────────────────────────────────────────────────

void UpdateChecker::checkAsync()
{
    if (m_state == State::Checking)
        return;
    setState(State::Checking);

    QNetworkRequest req(QUrl(QString::fromLatin1(kApiUrl)));
    req.setTransferTimeout(10000);
    req.setRawHeader("Accept", "application/vnd.github+json");
    req.setRawHeader("User-Agent", "numi-kde/" NUMI_KDE_VERSION);

    QNetworkReply *reply = m_nam->get(req);
    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            setState(State::Idle);
            emit checkFinished(false);
            return;
        }

        QSettings s(QStringLiteral("numi-kde"), QStringLiteral("numi-kde"));
        s.beginGroup(QStringLiteral("Updates"));
        s.setValue(QStringLiteral("lastCheck"), QDateTime::currentDateTimeUtc());
        s.endGroup();

        const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        const QString tag = obj.value(QStringLiteral("tag_name")).toString();
        const QString htmlUrl = obj.value(QStringLiteral("html_url")).toString();

        const bool found = !tag.isEmpty()
                        && isNewerVersion(tag, QStringLiteral(NUMI_KDE_VERSION));

        if (found && htmlUrl.startsWith(QStringLiteral("https://github.com/DMYTROSKORIN/numi-kde"))) {
            // Find the RPM asset download URL
            const QJsonArray assets = obj.value(QStringLiteral("assets")).toArray();
            for (const QJsonValue &a : assets) {
                const QJsonObject ao = a.toObject();
                const QString name = ao.value(QStringLiteral("name")).toString();
                if (name.startsWith(QStringLiteral("numi-kde-")) && name.endsWith(QStringLiteral(".rpm"))) {
                    const QString dlUrl = ao.value(QStringLiteral("browser_download_url")).toString();
                    if (dlUrl.startsWith(QStringLiteral("https://")))
                        m_downloadUrl = QUrl(dlUrl);
                    break;
                }
            }

            m_availableVersion = tag;
            emit availableVersionChanged();
            setState(State::UpdateAvailable);
            emit updateAvailable(tag, QUrl(htmlUrl));

            if (m_autoDownload && m_downloadUrl.isValid())
                startDownload();
        } else {
            setState(State::Idle);
        }

        emit checkFinished(found);
    });
}

// ── Download ─────────────────────────────────────────────────────────────────

void UpdateChecker::startDownload()
{
    if (m_state != State::UpdateAvailable || !m_downloadUrl.isValid())
        return;

    const QDir cacheDir(
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    cacheDir.mkpath(QStringLiteral("."));

    // Version string without leading 'v'
    QString ver = m_availableVersion;
    if (ver.startsWith(QLatin1Char('v')))
        ver = ver.mid(1);

    const QString arch = QSysInfo::currentCpuArchitecture();
    const QString rpmName =
        QStringLiteral("numi-kde-%1-%2.rpm").arg(ver, arch);
    m_rpmPath = cacheDir.filePath(rpmName);

    // Remove stale partial download if any
    QFile::remove(m_rpmPath + QStringLiteral(".part"));

    m_dlFile = new QFile(m_rpmPath + QStringLiteral(".part"), this);
    if (!m_dlFile->open(QIODevice::WriteOnly)) {
        delete m_dlFile;
        m_dlFile = nullptr;
        setState(State::Error);
        return;
    }

    setState(State::Downloading);
    setDownloadProgress(0);

    QNetworkRequest req(m_downloadUrl);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setRawHeader("User-Agent", "numi-kde/" NUMI_KDE_VERSION);

    QNetworkReply *reply = m_nam->get(req);

    QObject::connect(reply, &QNetworkReply::readyRead, this, [this, reply]() {
        if (m_dlFile)
            m_dlFile->write(reply->readAll());
    });

    QObject::connect(reply, &QNetworkReply::downloadProgress, this,
        [this](qint64 received, qint64 total) {
            if (total > 0)
                setDownloadProgress(int(received * 100 / total));
        });

    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        const QString partPath = m_rpmPath + QStringLiteral(".part");
        if (m_dlFile) {
            m_dlFile->close();
            delete m_dlFile;
            m_dlFile = nullptr;
        }

        if (reply->error() != QNetworkReply::NoError) {
            QFile::remove(partPath);
            setState(State::Error);
            return;
        }

        if (!QFile::rename(partPath, m_rpmPath)) {
            QFile::remove(partPath);
            setState(State::Error);
            return;
        }

        setState(State::DownloadReady);
        emit downloadReady(m_availableVersion);
    });
}

// ── Install via PackageKit D-Bus API ─────────────────────────────────────────

void UpdateChecker::installUpdate()
{
    if (m_state != State::DownloadReady || m_pkTransaction)
        return;

    setState(State::Installing);
    m_lastError.clear();
    setDownloadProgress(0);

    // TransactionFlagNone allows installing unsigned (untrusted) local RPMs.
    // PackageKit daemon handles polkit authentication — KDE polkit agent shows
    // the confirmation dialog automatically via D-Bus.
    PackageKit::Transaction *t = PackageKit::Daemon::installFile(
        m_rpmPath,
        PackageKit::Transaction::TransactionFlagNone);
    m_pkTransaction = t;

    QObject::connect(t, &PackageKit::Transaction::percentageChanged, this, [this, t]() {
        const uint pct = t->percentage();
        if (pct <= 100)
            setDownloadProgress(static_cast<int>(pct));
    });

    QObject::connect(t, &PackageKit::Transaction::errorCode, this,
        [this](PackageKit::Transaction::Error, const QString &details) {
            if (m_lastError.isEmpty())
                m_lastError = details;
        });

    // Transaction auto-deletes itself after finished() — do not delete manually.
    QObject::connect(t, &PackageKit::Transaction::finished, this,
        [this](PackageKit::Transaction::Exit status, uint) {
            m_pkTransaction = nullptr;
            QFile::remove(m_rpmPath);

            if (status == PackageKit::Transaction::ExitSuccess) {
                setState(State::RestartRequired);
                emit installFinished(true);
            } else {
                setState(State::Error);
                emit installFinished(false);
            }
        });
}

// ── Restart ──────────────────────────────────────────────────────────────────

void UpdateChecker::restartApp()
{
    QProcess::startDetached(QCoreApplication::applicationFilePath(),
                            QCoreApplication::arguments());
    QCoreApplication::quit();
}
