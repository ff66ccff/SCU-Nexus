#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtQuickControls2/QQuickStyle>

#include "src/app/AppController.h"
#include "src/app/Router.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    app.setOrganizationName("SCU");
    app.setApplicationName("SCU_Nexus");
    app.setApplicationVersion("0.1.0");

    QQuickStyle::setStyle("Material");

    // 创建核心对象
    AppController appController;
    Router router;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("appController", &appController);
    engine.rootContext()->setContextProperty("router", &router);

    const QUrl url("qrc:/qml/App.qml");
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
                         if (!obj && url == objUrl)
                             QCoreApplication::exit(-1);
                     }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}