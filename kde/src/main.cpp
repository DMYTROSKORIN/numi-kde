#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QDesktopServices>
#include <QMetaObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QIcon>
#include <QDebug>
#include <QEvent>
#include <QEventLoop>
#include <QPalette>
#include <QTimer>
#include <cstdio>

#include <KDBusService>
#include <KNotification>
#include <KWindowSystem>

#include "documentmodel.h"
#include "engineclient.h"
#include "shortcutmanager.h"
#include "updatechecker.h"
#include "windowmemory.h"

#ifndef NUMI_KDE_VERSION
#define NUMI_KDE_VERSION "0.0.0"
#endif
#ifndef NUMI_KDE_APP_ID
#define NUMI_KDE_APP_ID "online.skorin.numi-kde"
#endif

static bool hasArgument(int argc, char *argv[], const char *argument)
{
    for (int i = 1; i < argc; ++i) {
        if (qstrcmp(argv[i], argument) == 0)
            return true;
    }
    return false;
}

// `--probe`: end-to-end check used after installation — spawns the real
// engine process and evaluates through it, exactly like the GUI does.
static int runProbe(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("numi-kde"));
    app.setOrganizationName(QStringLiteral("numi-kde"));

    EngineClient engine;
    if (!engine.start()) {
        std::fprintf(stderr, "numi-kde %s probe failed: engine %s did not start\n",
                     NUMI_KDE_VERSION, qPrintable(EngineClient::enginePath()));
        return 1;
    }

    QList<LineResult> results;
    QEventLoop loop;
    QObject::connect(&engine, &EngineClient::evaluated, &loop, [&](quint64, const QList<LineResult> &lines) {
        results = lines;
        loop.quit();
    });
    QTimer::singleShot(15000, &loop, &QEventLoop::quit);
    engine.evaluate(1, QStringLiteral("2 + 2\n1 km to m"));
    loop.exec();

    const bool ok = results.size() == 2
                 && results.at(0).ok
                 && results.at(0).hasNumericValue
                 && results.at(0).numericValue == 4.0
                 && results.at(1).ok
                 && results.at(1).hasNumericValue
                 && results.at(1).numericValue == 1000.0;

    if (ok) {
        std::printf("numi-kde %s probe ok (engine %s)\n", NUMI_KDE_VERSION, qPrintable(engine.engineVersion()));
        return 0;
    }

    std::fprintf(stderr, "numi-kde %s probe failed (%d lines)\n", NUMI_KDE_VERSION, int(results.size()));
    return 1;
}

static void showWindow(QWindow *win, DocumentModel *model)
{
    if (!win) return;
    if (model)
        model->prepareShow();
    win->setWindowStates(win->windowStates() & ~Qt::WindowMinimized);
    win->show();
    // KWindowSystem applies the xdg-activation token handed to us by
    // kglobalacceld / the tray, which plain requestActivate() would miss.
    KWindowSystem::activateWindow(win);
}

// Toggle main window show/hide
static void toggleWindow(QWindow *win, DocumentModel *model)
{
    if (!win) return;

    const bool minimized = win->windowState() & Qt::WindowMinimized;
    if (win->isVisible() && !minimized) {
        if (!QMetaObject::invokeMethod(win, "hideWindow"))
            win->hide();
    } else {
        showWindow(win, model);
    }
}

// The tray badge exists in two hand-made variants: the original (light glyph on
// a dark disc) for dark colour schemes and its inverse for light ones. Plasma's
// colour scheme reaches us through the application palette.
static QIcon trayIconForPalette()
{
    const bool lightScheme = QGuiApplication::palette().color(QPalette::Window).lightness() > 128;
    return QIcon(lightScheme ? QStringLiteral(":/icons/numi-kde-tray-light.png")
                             : QStringLiteral(":/icons/numi-kde-tray.png"));
}

// Swaps the tray icon when the user changes the colour scheme while we run.
class TrayThemeWatcher : public QObject
{
public:
    TrayThemeWatcher(QSystemTrayIcon *tray, QObject *parent = nullptr) : QObject(parent), m_tray(tray) {}
protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event->type() == QEvent::ApplicationPaletteChange || event->type() == QEvent::ThemeChange)
            m_tray->setIcon(trayIconForPalette());
        return QObject::eventFilter(watched, event);
    }
private:
    QSystemTrayIcon *m_tray;
};

static KNotification *notify(const QString &event, const QString &title, const QString &text)
{
    auto *n = new KNotification(event, KNotification::CloseOnTimeout);
    n->setComponentName(QStringLiteral("numi-kde"));
    n->setTitle(title);
    n->setText(text);
    n->setIconName(QStringLiteral(NUMI_KDE_APP_ID));
    return n;
}

int main(int argc, char *argv[])
{
    if (hasArgument(argc, argv, "--version")) {
        std::printf("numi-kde %s\n", NUMI_KDE_VERSION);
        return 0;
    }
    if (hasArgument(argc, argv, "--probe"))
        return runProbe(argc, argv);
    if (hasArgument(argc, argv, "--help")) {
        std::printf("Usage: numi-kde [--version] [--probe] [--hidden]\n");
        return 0;
    }

    // DocumentModel MUST be declared before engine so it outlives QML's
    // Component.onDestruction (stack is unwound in reverse: engine first, then model)
    QApplication app(argc, argv);
    // applicationName/organizationName stay "numi-kde" so QSettings, the
    // KGlobalAccel component and the history location are unchanged.
    app.setApplicationName(QStringLiteral("numi-kde"));
    app.setApplicationDisplayName(QStringLiteral("Numi-KDE"));
    app.setOrganizationName(QStringLiteral("numi-kde"));
    app.setOrganizationDomain(QStringLiteral("skorin.online"));
    app.setDesktopFileName(QStringLiteral(NUMI_KDE_APP_ID));
    app.setApplicationVersion(QStringLiteral(NUMI_KDE_VERSION));
    app.setQuitOnLastWindowClosed(false);   // stay alive in tray when window is closed
    const QIcon appIcon = QIcon::fromTheme(QStringLiteral(NUMI_KDE_APP_ID),
                                           QIcon(QStringLiteral(":/icons/numi-kde.png")));
    app.setWindowIcon(appIcon);

    // Single instance: a second launch (menu, autostart, `numi-kde` from a
    // shell) forwards its arguments to us and exits.
    KDBusService service(KDBusService::Unique);

    // Wayland window positions: KWin's own "remember position" rule is never
    // persisted for Wayland windows, so our KWin script reports and restores
    // geometry through this D-Bus object instead.
    WindowMemory windowMemory;
    windowMemory.registerOnSessionBus();
    if (KWindowSystem::isPlatformWayland())
        WindowMemory::ensureKWinScriptLoaded();

    QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));

    DocumentModel documentModel;
    documentModel.migrateLegacyState();
    const bool startHidden = hasArgument(argc, argv, "--hidden");
    QAction showHideAction;
    showHideAction.setObjectName(QStringLiteral("toggle-window"));
    ShortcutManager shortcutManager(&showHideAction);

    UpdateChecker updateChecker;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("documentModel"), &documentModel);
    engine.rootContext()->setContextProperty(QStringLiteral("shortcutManager"), &shortcutManager);
    engine.rootContext()->setContextProperty(QStringLiteral("updateChecker"), &updateChecker);
    QObject::connect(&engine, &QQmlApplicationEngine::warnings,
        [](const QList<QQmlError> &warnings) {
            for (const QQmlError &warning : warnings)
                qWarning().noquote() << warning.toString();
        });
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() {
            qCritical() << "Failed to create QML root object";
            QCoreApplication::exit(1);
        },
        Qt::QueuedConnection);

    engine.loadFromModule(QStringLiteral("NumiKde"), QStringLiteral("Main"));
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "No QML root objects loaded";
        return 1;
    }

    // Locate main window from QML root objects
    QWindow *mainWindow = nullptr;
    for (QObject *obj : engine.rootObjects()) {
        if (auto *w = qobject_cast<QWindow *>(obj)) {
            mainWindow = w;
            mainWindow->setIcon(appIcon);
            break;
        }
    }

    if (mainWindow) {
        documentModel.setKeepAbove(mainWindow->property("alwaysOnTop").toBool());
        if (!startHidden)
            showWindow(mainWindow, &documentModel);
    }

    // Second instance asked for the window unless it was an autostart (--hidden).
    QObject::connect(&service, &KDBusService::activateRequested,
        [mainWindow, &documentModel](const QStringList &arguments, const QString &) {
            if (arguments.contains(QStringLiteral("--hidden")))
                return;
            showWindow(mainWindow, &documentModel);
        });

    // ── System tray ──────────────────────────────────────────────────────
    QSystemTrayIcon tray;
    tray.setIcon(trayIconForPalette());
    tray.setToolTip(QStringLiteral("Numi-KDE"));
    TrayThemeWatcher trayThemeWatcher(&tray);
    app.installEventFilter(&trayThemeWatcher);

    QMenu trayMenu;
    trayMenu.addAction(&showHideAction);
    trayMenu.addSeparator();
    QAction *checkUpdatesAction = trayMenu.addAction(QStringLiteral("Check for Updates"));
    trayMenu.addSeparator();
    QAction *quitAction = trayMenu.addAction(QStringLiteral("Quit Numi-KDE"));
    tray.setContextMenu(&trayMenu);
    tray.show();

    // Tray left-click toggles window
    QObject::connect(&tray, &QSystemTrayIcon::activated,
        [mainWindow, &documentModel](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger)
                toggleWindow(mainWindow, &documentModel);
        });

    QObject::connect(&showHideAction, &QAction::triggered,
        [mainWindow, &documentModel]() { toggleWindow(mainWindow, &documentModel); });

    QObject::connect(quitAction, &QAction::triggered, &app, &QCoreApplication::quit);

    // ── Update checker ────────────────────────────────────────────────────

    // Dynamic tray actions (hidden until relevant state)
    QAction *installUpdateAction = new QAction(QStringLiteral("Install Update"), &trayMenu);
    installUpdateAction->setVisible(false);
    trayMenu.insertAction(checkUpdatesAction, installUpdateAction);

    QAction *restartAction = new QAction(QStringLiteral("Restart to Apply Update"), &trayMenu);
    restartAction->setVisible(false);
    trayMenu.insertAction(checkUpdatesAction, restartAction);

    QAction *separatorUpdate = trayMenu.insertSeparator(checkUpdatesAction);
    separatorUpdate->setVisible(false);

    // Manual "Check for Updates" — replaced with release URL once update found
    bool manualCheckInProgress = false;
    QObject::connect(checkUpdatesAction, &QAction::triggered,
        [&updateChecker, &tray, &manualCheckInProgress]() {
            manualCheckInProgress = true;
            updateChecker.checkAsync();
            tray.showMessage(QStringLiteral("Numi-KDE"),
                             QStringLiteral("Checking for updates…"),
                             QSystemTrayIcon::Information, 2000);
        });

    QObject::connect(&updateChecker, &UpdateChecker::checkFinished,
        [&tray, &manualCheckInProgress](bool found) {
            if (manualCheckInProgress && !found) {
                tray.showMessage(QStringLiteral("Numi-KDE"),
                                 QStringLiteral("You are using the latest version."),
                                 QSystemTrayIcon::Information, 3000);
            }
            manualCheckInProgress = false;
        });

    // Update available — show release link in tray menu. Installation starts
    // on its own when auto-install is enabled; nothing is shown to the user.
    QObject::connect(&updateChecker, &UpdateChecker::updateAvailable,
        [&tray, &updateChecker, checkUpdatesAction, installUpdateAction, separatorUpdate]
        (const QString &version, const QUrl &url) {
            checkUpdatesAction->setText(QStringLiteral("Update available: %1").arg(version));
            tray.setToolTip(QStringLiteral("Numi-KDE — update %1 available").arg(version));
            QObject::disconnect(checkUpdatesAction, &QAction::triggered, nullptr, nullptr);
            QObject::connect(checkUpdatesAction, &QAction::triggered,
                [url]() { QDesktopServices::openUrl(url); });
            if (!updateChecker.autoInstallUpdates()) {
                separatorUpdate->setVisible(true);
                installUpdateAction->setVisible(true);
                installUpdateAction->setText(QStringLiteral("Install Update %1").arg(version));
            }
        });

    QObject::connect(installUpdateAction, &QAction::triggered,
        [&updateChecker, installUpdateAction]() {
            installUpdateAction->setEnabled(false);
            installUpdateAction->setText(QStringLiteral("Installing…"));
            updateChecker.installUpdate();
        });

    // Restart policy: restart immediately when the window is hidden (the user
    // is not looking), otherwise wait for the next hide and offer a
    // notification action in the meantime. The window is never taken away.
    bool restartPending = false;
    auto restartNow = [&updateChecker, &restartPending]() {
        restartPending = false;
        updateChecker.restartApp();
    };

    if (mainWindow) {
        QObject::connect(mainWindow, &QWindow::visibleChanged,
            [&restartPending, restartNow](bool visible) {
                if (!visible && restartPending)
                    restartNow();
            });
    }

    QObject::connect(&updateChecker, &UpdateChecker::installFinished,
        [&updateChecker, mainWindow, installUpdateAction, restartAction, separatorUpdate,
         &restartPending, restartNow](bool success) {
            installUpdateAction->setEnabled(true);
            if (success) {
                installUpdateAction->setVisible(false);
                const bool windowInUse = mainWindow && mainWindow->isVisible();
                if (!windowInUse) {
                    restartNow();
                    return;
                }
                restartPending = true;
                separatorUpdate->setVisible(true);
                restartAction->setVisible(true);
                KNotification *n = notify(QStringLiteral("updateInstalled"),
                    QStringLiteral("Update %1 installed").arg(updateChecker.availableVersion()),
                    QStringLiteral("Numi-KDE will restart as soon as you hide the window."));
                KNotificationAction *restart = n->addAction(QStringLiteral("Restart now"));
                QObject::connect(restart, &KNotificationAction::activated, restartNow);
                n->sendEvent();
            } else {
                separatorUpdate->setVisible(true);
                installUpdateAction->setVisible(true);
                installUpdateAction->setText(QStringLiteral("Install Update (retry)…"));
                const QString err = updateChecker.lastError();
                KNotification *n = notify(QStringLiteral("updateFailed"),
                    QStringLiteral("Update failed"),
                    err.isEmpty()
                        ? QStringLiteral("Update installation failed. Try again from the tray menu.")
                        : err);
                n->sendEvent();
            }
        });

    QObject::connect(restartAction, &QAction::triggered, restartNow);

    // First start after an update: say so once, then get out of the way.
    {
        const QString previous = UpdateChecker::consumeLastRunVersion();
        if (!previous.isEmpty() && previous != QStringLiteral(NUMI_KDE_VERSION)) {
            notify(QStringLiteral("updated"),
                   QStringLiteral("Numi-KDE updated"),
                   QStringLiteral("Now running version %1.").arg(QStringLiteral(NUMI_KDE_VERSION)))
                ->sendEvent();
        }
    }

    // Auto-check once per 24 hours on startup
    if (updateChecker.shouldAutoCheck())
        updateChecker.checkAsync();

    // Save session before quit
    QObject::connect(&app, &QCoreApplication::aboutToQuit,
        [&documentModel]() { documentModel.saveSession(); });

    return app.exec();
}
