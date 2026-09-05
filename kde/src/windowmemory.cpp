#include "windowmemory.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QSettings>

namespace {
const QLatin1String kGroup("WindowMemory");
const QLatin1String kScriptId("numi-kde-window-memory");
}

WindowMemory::WindowMemory(QObject *parent)
    : QObject(parent)
{
}

bool WindowMemory::registerOnSessionBus()
{
    return QDBusConnection::sessionBus().registerObject(
        QStringLiteral("/WindowMemory"), this, QDBusConnection::ExportAllSlots);
}

QString WindowMemory::keyFor(const QString &window)
{
    // Captions are stable ("Numi-KDE", "Numi-KDE Settings"); keep keys simple.
    QString key = window.trimmed().toLower();
    key.replace(QLatin1Char(' '), QLatin1Char('-'));
    key.remove(QLatin1Char('/'));
    return key.isEmpty() ? QStringLiteral("main") : key;
}

void WindowMemory::rememberGeometry(const QString &window, int x, int y, int width, int height)
{
    Q_UNUSED(width)
    Q_UNUSED(height)
    QSettings s(QStringLiteral("numi-kde"), QStringLiteral("numi-kde"));
    s.beginGroup(kGroup);
    s.setValue(keyFor(window), QStringLiteral("%1,%2").arg(x).arg(y));
    s.endGroup();
}

QString WindowMemory::savedGeometry(const QString &window) const
{
    QSettings s(QStringLiteral("numi-kde"), QStringLiteral("numi-kde"));
    s.beginGroup(kGroup);
    const QString value = s.value(keyFor(window)).toString();
    s.endGroup();
    return value;
}

void WindowMemory::ensureKWinScriptLoaded()
{
    QDBusInterface scripting(QStringLiteral("org.kde.KWin"),
                             QStringLiteral("/Scripting"),
                             QStringLiteral("org.kde.kwin.Scripting"));
    if (!scripting.isValid())
        return;
    const QDBusReply<bool> loaded = scripting.call(QStringLiteral("isScriptLoaded"), QString(kScriptId));
    if (loaded.isValid() && loaded.value())
        return;

    // Enabled-by-default scripts are picked up when KWin re-reads its config.
    QDBusInterface kwin(QStringLiteral("org.kde.KWin"),
                        QStringLiteral("/KWin"),
                        QStringLiteral("org.kde.KWin"));
    if (kwin.isValid())
        kwin.call(QStringLiteral("reconfigure"));
}
