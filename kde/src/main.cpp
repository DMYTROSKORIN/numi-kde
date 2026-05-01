#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QIcon>
#include <QKeySequence>
#include <QDebug>
#include <QStandardPaths>

#ifdef NUMI_KDE_HAVE_GLOBAL_ACCEL
#include <KGlobalAccel>
#endif

#include "documentmodel.h"
#include "shortcutmanager.h"

// Toggle main window show/hide
static void toggleWindow(QWindow *win)
{
    if (!win) return;
    if (win->isVisible() && win->windowState() != Qt::WindowMinimized) {
        win->hide();
    } else {
        win->show();
        win->raise();
        win->requestActivate();
    }
}

int main(int argc, char *argv[])
{
    // DocumentModel MUST be declared before engine so it outlives QML's
    // Component.onDestruction (stack is unwound in reverse: engine first, then model)
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("numi-kde"));
    if (!QStandardPaths::locate(QStandardPaths::ApplicationsLocation,
                                QStringLiteral("org.skorin.numi-kde.desktop")).isEmpty()) {
        app.setDesktopFileName(QStringLiteral("org.skorin.numi-kde"));
    }
    app.setOrganizationName(QStringLiteral("skorin"));
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

    // Save session before quit
    QObject::connect(&app, &QCoreApplication::aboutToQuit,
        [&documentModel]() { documentModel.saveSession(); });

    return app.exec();
}
