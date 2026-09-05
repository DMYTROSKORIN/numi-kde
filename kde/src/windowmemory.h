#pragma once

#include <QObject>
#include <QString>

/**
 * D-Bus endpoint used by the KWin script `numi-kde-window-memory`
 * (kde/resources/kwin-script) to persist window positions on Wayland.
 *
 * Exported at online.skorin.numi-kde /WindowMemory,
 * interface online.skorin.numi_kde.WindowMemory.
 */
class WindowMemory : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "online.skorin.numi_kde.WindowMemory")

public:
    explicit WindowMemory(QObject *parent = nullptr);

    /// Registers /WindowMemory on the session bus (service name comes from KDBusService).
    bool registerOnSessionBus();

    /// Makes sure KWin has our script loaded: asks KWin to reconfigure when it
    /// is installed but not loaded yet (e.g. right after the first install).
    static void ensureKWinScriptLoaded();

public slots:
    /// Called by the KWin script after the user moved or closed a window.
    void rememberGeometry(const QString &window, int x, int y, int width, int height);
    /// Returns "x,y" or an empty string when nothing has been saved for @p window.
    QString savedGeometry(const QString &window) const;

private:
    static QString keyFor(const QString &window);
};
