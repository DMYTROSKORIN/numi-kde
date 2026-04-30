#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "documentmodel.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("numi-kde"));
    app.setOrganizationName(QStringLiteral("skorin"));

    QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));

    QQmlApplicationEngine engine;
    DocumentModel documentModel;
    engine.rootContext()->setContextProperty(QStringLiteral("documentModel"), &documentModel);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(1); },
        Qt::QueuedConnection);

    engine.loadFromModule(QStringLiteral("NumiKde"), QStringLiteral("Main"));

    return app.exec();
}
