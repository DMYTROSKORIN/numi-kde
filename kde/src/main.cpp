#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("numi-kde"));
    app.setOrganizationName(QStringLiteral("skorin"));

    QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(1); },
        Qt::QueuedConnection);

    engine.loadFromModule(QStringLiteral("NumiKde"), QStringLiteral("Main"));

    return app.exec();
}
