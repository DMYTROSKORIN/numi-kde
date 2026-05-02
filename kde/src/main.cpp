#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QDesktopServices>
#include <QSettings>
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
                 && results.at(0).result == QStringLiteral("4")
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

static void saveWindowPosition(QWindow *win)
{
    QSettings s(QStringLiteral("numi-kde"), QStringLiteral("numi-kde"));
    s.beginGroup(QStringLiteral("Window"));
    s.setValue(QStringLiteral("savedX"), win->x());
    s.setValue(QStringLiteral("savedY"), win->y());
    s.endGroup();
}

// Toggle main window show/hide
static void toggleWindow(QWindow *win)
{
    if (!win) return;
    if (win->isVisible() && win->windowState() != Qt::WindowMinimized) {
        saveWindowPosition(win);
        win->showMinimized();
    } else {
        win->showNormal();
        // Restore position after show. X11 honours setPosition(); on Wayland
        // the compositor controls placement, but the KWin rule handles it there.
        QSettings s(QStringLiteral("numi-kde"), QStringLiteral("numi-kde"));
        s.beginGroup(QStringLiteral("Window"));
        const int x = s.value(QStringLiteral("savedX"), -1).toInt();
        const int y = s.value(QStringLiteral("savedY"), -1).toInt();
        s.endGroup();
        if (x >= 0 && y >= 0)
            win->setPosition(x, y);
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
    if (hasArgument(argc, argv, "--help")) {
        std::printf("Usage: numi-kde [--version] [--probe]\n");
        return 0;
    }

    // DocumentModel MUST be declared before engine so it outlives QML's
    // Component.onDestruction (stack is unwound in reverse: engine first, then model)
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("numi-kde"));
    if (!QStandardPaths::locate(QStandardPaths::ApplicationsLocation,
                                QStringLiteral("numi-kde.desktop")).isEmpty()) {
        app.setDesktopFileName(QStringLiteral("numi-kde"));
    }
    app.setOrganizationName(QStringLiteral("numi-kde"));
    app.setQuitOnLastWindowClosed(false);   // stay alive in tray when window is closed
    const QIcon appIcon(QStringLiteral(":/icons/numi-kde.png"));
    const QIcon trayIcon(QStringLiteral(":/icons/numi-kde-tray.png"));
    app.setWindowIcon(appIcon);

    QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));

    DocumentModel documentModel;
    QAction showHideAction;
    showHideAction.setObjectName(QStringLiteral("toggle-window"));
    ShortcutManager shortcutManager(&showHideAction);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("documentModel"), &documentModel);
    engine.rootContext()->setContextProperty(QStringLiteral("shortcutManager"), &shortcutManager);
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
        [mainWindow](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger)
                toggleWindow(mainWindow);
        });

    QObject::connect(&showHideAction, &QAction::triggered,
        [mainWindow]() { toggleWindow(mainWindow); });

    QObject::connect(quitAction, &QAction::triggered, &app, &QCoreApplication::quit);

    // ── Update checker ────────────────────────────────────────────────────
    UpdateChecker updateChecker;

    // When update available: replace action text and open releases page on click
    QObject::connect(&updateChecker, &UpdateChecker::updateAvailable,
        [&tray, checkUpdatesAction](const QString &version, const QUrl &url) {
            const QString label = QStringLiteral("Update available: %1").arg(version);
            checkUpdatesAction->setText(label);
            checkUpdatesAction->setData(url);
            tray.setToolTip(QStringLiteral("Numi-KDE — ") + label);
            // Disconnect previous click handlers and open release URL
            QObject::disconnect(checkUpdatesAction, &QAction::triggered, nullptr, nullptr);
            QObject::connect(checkUpdatesAction, &QAction::triggered, [url]() {
                QDesktopServices::openUrl(url);
            });
        });

    // Manual check: restore label if no update found after an explicit click
    bool manualCheckInProgress = false;
    QObject::connect(checkUpdatesAction, &QAction::triggered, [&updateChecker, &tray, &manualCheckInProgress]() {
        manualCheckInProgress = true;
        updateChecker.checkAsync();
        tray.showMessage(QStringLiteral("Numi-KDE"), 
                         QStringLiteral("Checking for updates..."), 
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

    QObject::connect(&updateChecker, &UpdateChecker::updateAvailable,
        [&tray](const QString &version, const QUrl &url) {
            tray.showMessage(QStringLiteral("Numi-KDE"),
                             QStringLiteral("A new version is available: %1").arg(version),
                             QSystemTrayIcon::Information, 5000);
        });

    // Auto-check once per 24 hours on startup
    if (updateChecker.shouldAutoCheck())
        updateChecker.checkAsync();

    // Save session before quit
    QObject::connect(&app, &QCoreApplication::aboutToQuit,
        [&documentModel]() { documentModel.saveSession(); });

    return app.exec();
}
