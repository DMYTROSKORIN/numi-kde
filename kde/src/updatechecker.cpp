#include "updatechecker.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

#ifndef NUMI_KDE_VERSION
#define NUMI_KDE_VERSION "0.0.0"
#endif

static const char kApiUrl[] =
    "https://api.github.com/repos/DMYTROSKORIN/numi-kde/releases/latest";
static const char kReleasePagePrefix[] = "https://github.com/DMYTROSKORIN/numi-kde";
static const char kHelperPath[] = "/usr/libexec/numi-kde-install-update";

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

static QSettings updateSettings()
{
    return QSettings(QStringLiteral("numi-kde"), QStringLiteral("numi-kde"));
}

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent), m_nam(new QNetworkAccessManager(this))
{
    QSettings s = updateSettings();
    s.beginGroup(QStringLiteral("Updates"));
    // "autoDownload" is the pre-0.1.81 key; honour it once, then use autoInstall.
    m_autoInstall = s.value(QStringLiteral("autoInstall"),
                            s.value(QStringLiteral("autoDownload"), true)).toBool();
    s.endGroup();

    // Check once per hour; the actual 24 h gate is inside shouldAutoCheck().
    m_periodicTimer = new QTimer(this);
    m_periodicTimer->setInterval(60 * 60 * 1000);
    QObject::connect(m_periodicTimer, &QTimer::timeout, this, [this]() {
        if (shouldAutoCheck())
            checkAsync();
    });
    m_periodicTimer->start();
}

bool UpdateChecker::shouldAutoCheck()
{
    QSettings s = updateSettings();
    s.beginGroup(QStringLiteral("Updates"));
    const QDateTime last = s.value(QStringLiteral("lastCheck")).toDateTime();
    s.endGroup();
    return !last.isValid() || last.secsTo(QDateTime::currentDateTimeUtc()) >= 86400;
}

QString UpdateChecker::consumeLastRunVersion()
{
    QSettings s = updateSettings();
    s.beginGroup(QStringLiteral("Updates"));
    const QString previous = s.value(QStringLiteral("lastRunVersion")).toString();
    s.setValue(QStringLiteral("lastRunVersion"), QStringLiteral(NUMI_KDE_VERSION));
    s.endGroup();
    return previous;
}

void UpdateChecker::setAutoInstallUpdates(bool enabled)
{
    if (m_autoInstall == enabled)
        return;
    m_autoInstall = enabled;
    QSettings s = updateSettings();
    s.beginGroup(QStringLiteral("Updates"));
    s.setValue(QStringLiteral("autoInstall"), enabled);
    s.endGroup();
    emit autoInstallUpdatesChanged();
}

void UpdateChecker::setState(State s)
{
    if (m_state == s)
        return;
    m_state = s;
    emit stateChanged(s);
}

// ── Version check ────────────────────────────────────────────────────────────

void UpdateChecker::checkAsync()
{
    if (m_state == State::Checking || m_state == State::Installing
            || m_state == State::RestartRequired)
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
            setState(m_availableVersion.isEmpty() ? State::Idle : State::UpdateAvailable);
            emit checkFinished(false);
            return;
        }

        {
            QSettings s = updateSettings();
            s.beginGroup(QStringLiteral("Updates"));
            s.setValue(QStringLiteral("lastCheck"), QDateTime::currentDateTimeUtc());
            s.endGroup();
        }

        const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        const QString tag = obj.value(QStringLiteral("tag_name")).toString();
        const QString htmlUrl = obj.value(QStringLiteral("html_url")).toString();

        static const QRegularExpression tagPattern(QStringLiteral("^v?\\d+\\.\\d+\\.\\d+$"));
        const bool found = tagPattern.match(tag).hasMatch()
                        && htmlUrl.startsWith(QString::fromLatin1(kReleasePagePrefix))
                        && isNewerVersion(tag, QStringLiteral(NUMI_KDE_VERSION));

        if (found) {
            m_availableVersion = tag;
            emit availableVersionChanged();
            setState(State::UpdateAvailable);
            emit updateAvailable(tag, QUrl(htmlUrl));
            emit checkFinished(true);
            if (m_autoInstall)
                installUpdate();
        } else {
            setState(State::Idle);
            emit checkFinished(false);
        }
    });
}

// ── Install via pkexec + polkit helper ───────────────────────────────────────

void UpdateChecker::installUpdate()
{
    if (m_installer || m_availableVersion.isEmpty())
        return;
    if (m_state != State::UpdateAvailable && m_state != State::Error)
        return;

    if (!QFile::exists(QString::fromLatin1(kHelperPath))) {
        m_lastError = QStringLiteral("Update helper not found — please reinstall numi-kde.");
        setState(State::Error);
        emit installFinished(false);
        return;
    }

    const QString pkexec = QStandardPaths::findExecutable(QStringLiteral("pkexec"));
    if (pkexec.isEmpty()) {
        m_lastError = QStringLiteral("pkexec not found.");
        setState(State::Error);
        emit installFinished(false);
        return;
    }

    setState(State::Installing);
    m_lastError.clear();

    // The helper gets only the version number; it downloads and verifies the
    // RPM itself as root, so nothing writable by this user is ever installed.
    m_installer = new QProcess(this);
    m_installer->setProgram(pkexec);
    m_installer->setArguments({QString::fromLatin1(kHelperPath), m_availableVersion});

    QObject::connect(m_installer, &QProcess::readyReadStandardError, this, [this]() {
        const QString text = QString::fromUtf8(m_installer->readAllStandardError()).trimmed();
        if (!text.isEmpty())
            m_lastError = text.section(QLatin1Char('\n'), -1);
    });

    QObject::connect(m_installer, &QProcess::finished, this,
        [this](int exitCode, QProcess::ExitStatus status) {
            m_installer->deleteLater();
            m_installer = nullptr;

            if (status == QProcess::NormalExit && exitCode == 0) {
                setState(State::RestartRequired);
                emit installFinished(true);
            } else {
                if (m_lastError.isEmpty())
                    m_lastError = QStringLiteral("Installer exited with code %1.").arg(exitCode);
                setState(State::Error);
                emit installFinished(false);
            }
        });

    m_installer->start();
}

// ── Restart ──────────────────────────────────────────────────────────────────

void UpdateChecker::restartApp()
{
    // Prefer the installed binary by name: /proc/self/exe may point at the
    // replaced (deleted) inode after an RPM upgrade.
    QString exe = QStandardPaths::findExecutable(QStringLiteral("numi-kde"));
    if (exe.isEmpty())
        exe = QCoreApplication::applicationFilePath();

    // Always come back hidden: the update was applied in the background and
    // the user has not asked for the window.
    QProcess::startDetached(exe, {QStringLiteral("--hidden")});
    QCoreApplication::quit();
}
