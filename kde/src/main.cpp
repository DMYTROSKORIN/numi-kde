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
#include <QKeySequence>
#include <QDebug>
#include <QStandardPaths>
#include <cstdio>

#ifdef NUMI_KDE_HAVE_GLOBAL_ACCEL
#include <KGlobalAccel>
#endif

#include "documentmodel.h"
#include "qalcbridge.h"
#include "shortcutmanager.h"
#include "updatechecker.h"

#ifndef NUMI_KDE_VERSION
#define NUMI_KDE_VERSION "0.0.0"
#endif

static bool hasArgument(int argc, char *argv[], const char *argument)
{
    for (int i = 1; i < argc; ++i) {
        if (qstrcmp(argv[i], argument) == 0)
            return true;
    }
    return false;
}

static int runProbe(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("numi-kde"));
    app.setOrganizationName(QStringLiteral("numi-kde"));

    QalcBridge bridge;
    const auto results = bridge.evaluateDocument(QStringLiteral("2 + 2\n1 km to m"));
    const bool ok = results.size() == 2
                 && results.at(0).ok
                 && results.at(0).hasNumericValue
                 && results.at(0).numericValue == 4.0
                 && results.at(1).ok
                 && results.at(1).hasNumericValue
                 && results.at(1).numericValue == 1000.0;

    if (ok) {
        std::printf("numi-kde %s probe ok\n", NUMI_KDE_VERSION);
        return 0;
    }

    std::fprintf(stderr, "numi-kde %s probe failed\n", NUMI_KDE_VERSION);
    return 1;
}

// Toggle main window show/hide
static void toggleWindow(QWindow *win, DocumentModel *model)
{
    if (!win) return;

    // Keep the stable 0.1.27 toggle contract: a visible window hides, a hidden
    // or minimized window is shown. The QML minimize button hides to tray, so
    // Wayland compositor minimize state is not needed for the common path.
    const bool minimized = win->windowState() & Qt::WindowMinimized;

    if (win->isVisible() && !minimized) {
        if (!QMetaObject::invokeMethod(win, "hideWindow"))
            win->hide();
    } else {
        if (model)
            model->prepareShow();
        win->setWindowStates(win->windowStates() & ~Qt::WindowMinimized);
        win->show();
        win->raise();
        win->requestActivate();
    }
}

int main(int argc, char *argv[])
{
    if (hasArgument(argc, argv, "--version")) {
        std::printf("numi-kde %s\n", NUMI_KDE_VERSION);
        return 0;
    }
    if (hasArgument(argc, argv, "--probe"))
        return runProbe(argc, argv);
    if (hasArgument(argc, argv, "--hidden")) {
        // Just continue to start the app in background
    } else if (hasArgument(argc, argv, "--help")) {
        std::printf("Usage: numi-kde [--version] [--probe] [--hidden]\n");
        return 0;
    }

    // DocumentModel MUST be declared before engine so it outlives QML's
    // Component.onDestruction (stack is unwound in reverse: engine first, then model)
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("numi-kde"));
    app.setDesktopFileName(QStringLiteral("numi-kde"));
    app.setOrganizationName(QStringLiteral("numi-kde"));
    app.setQuitOnLastWindowClosed(false);   // stay alive in tray when window is closed
    const QIcon appIcon(QStringLiteral(":/icons/numi-kde.png"));
    const QIcon trayIcon(QStringLiteral(":/icons/numi-kde-tray.png"));
    app.setWindowIcon(appIcon);

    QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));

    DocumentModel documentModel;
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
        if (!startHidden) {
            documentModel.prepareShow();
            mainWindow->show();
            mainWindow->raise();
            mainWindow->requestActivate();
        }
    }

    // ── System tray ──────────────────────────────────────────────────────
    QSystemTrayIcon tray;
    tray.setIcon(trayIcon);
    tray.setToolTip(QStringLiteral("Numi-KDE"));

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

    // Update available — show release link in tray menu; auto-download starts silently
    QObject::connect(&updateChecker, &UpdateChecker::updateAvailable,
        [&tray, checkUpdatesAction](const QString &version, const QUrl &url) {
            checkUpdatesAction->setText(
                QStringLiteral("Update available: %1").arg(version));
            tray.setToolTip(QStringLiteral("Numi-KDE — update %1 available").arg(version));
            QObject::disconnect(checkUpdatesAction, &QAction::triggered, nullptr, nullptr);
            QObject::connect(checkUpdatesAction, &QAction::triggered,
                [url]() { QDesktopServices::openUrl(url); });
        });

    // State transitions: notify user when download starts or silently fails
    auto *prevUpdateState = new UpdateChecker::State(UpdateChecker::State::Idle);
    QObject::connect(&updateChecker, &UpdateChecker::stateChanged,
        [&tray, &updateChecker, prevUpdateState](UpdateChecker::State newState) {
            using S = UpdateChecker::State;
            if (newState == S::Downloading) {
                tray.showMessage(
                    QStringLiteral("Numi-KDE"),
                    QStringLiteral("Downloading update %1…").arg(updateChecker.availableVersion()),
                    QSystemTrayIcon::Information, 4000);
            } else if (newState == S::Error && *prevUpdateState == S::Downloading) {
                tray.showMessage(
                    QStringLiteral("Numi-KDE"),
                    QStringLiteral("Failed to download update. Open the tray menu to get it manually."),
                    QSystemTrayIcon::Warning, 6000);
            }
            *prevUpdateState = newState;
        });

    // Download complete — install automatically via PackageKit (polkit dialog will appear)
    QObject::connect(&updateChecker, &UpdateChecker::downloadReady,
        [&tray, &updateChecker, mainWindow, installUpdateAction, separatorUpdate](const QString &version) {
            separatorUpdate->setVisible(true);
            installUpdateAction->setVisible(true);
            installUpdateAction->setEnabled(false);
            installUpdateAction->setText(QStringLiteral("Installing %1…").arg(version));
            if (mainWindow)
                QMetaObject::invokeMethod(mainWindow, "hideWindow");
            tray.showMessage(
                QStringLiteral("Numi-KDE"),
                QStringLiteral("Installing update %1… Confirm the system dialog if prompted.").arg(version),
                QSystemTrayIcon::Information, 10000);
            updateChecker.installUpdate();
        });

    // Install action — retry only, shown when auto-install fails
    QObject::connect(installUpdateAction, &QAction::triggered,
        [&updateChecker, mainWindow, installUpdateAction, &tray]() {
            installUpdateAction->setEnabled(false);
            installUpdateAction->setText(QStringLiteral("Installing…"));
            if (mainWindow)
                QMetaObject::invokeMethod(mainWindow, "hideWindow");
            tray.showMessage(
                QStringLiteral("Numi-KDE"),
                QStringLiteral("Installing update — confirm the system dialog if prompted."),
                QSystemTrayIcon::Information, 10000);
            updateChecker.installUpdate();
        });

    // Install complete — offer restart or allow retry
    QObject::connect(&updateChecker, &UpdateChecker::installFinished,
        [&tray, &updateChecker, installUpdateAction, restartAction, separatorUpdate](bool success) {
            installUpdateAction->setEnabled(true);
            if (success) {
                installUpdateAction->setVisible(false);
                restartAction->setVisible(true);
                QObject::disconnect(&tray, &QSystemTrayIcon::messageClicked, nullptr, nullptr);
                QObject::connect(&tray, &QSystemTrayIcon::messageClicked,
                    [restartAction]() { restartAction->trigger(); });
                tray.showMessage(
                    QStringLiteral("Numi-KDE"),
                    QStringLiteral("Update installed. Click here or use tray to restart."),
                    QSystemTrayIcon::Information, 8000);
            } else {
                installUpdateAction->setText(QStringLiteral("Install Update (retry)…"));
                const QString err = updateChecker.lastError();
                tray.showMessage(
                    QStringLiteral("Numi-KDE"),
                    err.isEmpty()
                        ? QStringLiteral("Update installation failed. Try again from the tray menu.")
                        : QStringLiteral("Update installation failed: %1").arg(err),
                    QSystemTrayIcon::Warning, 6000);
            }
        });

    QObject::connect(restartAction, &QAction::triggered,
        &updateChecker, &UpdateChecker::restartApp);

    // Auto-check once per 24 hours on startup
    if (updateChecker.shouldAutoCheck())
        updateChecker.checkAsync();

    // Save session before quit
    QObject::connect(&app, &QCoreApplication::aboutToQuit,
        [&documentModel]() { documentModel.saveSession(); });

    return app.exec();
}
