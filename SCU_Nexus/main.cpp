#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtQuickControls2/QQuickStyle>

#include "src/app/AppController.h"
#include "src/app/Router.h"
#include "src/app/ThemeManager.h"
#include "src/app/ToastManager.h"

// 初始化应用运行环境并进入主事件循环。
int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    app.setOrganizationName("SCU");
    app.setApplicationName("SCU_Nexus");
    app.setApplicationVersion("0.1.0");

    QQuickStyle::setStyle("Fusion");

    AppController appController;
    Router router;
    ThemeManager themeManager;
    ToastManager toastManager;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("appController", &appController);
    engine.rootContext()->setContextProperty("router", &router);
    engine.rootContext()->setContextProperty("themeManager", &themeManager);
    engine.rootContext()->setContextProperty("toastManager", &toastManager);

    const QUrl url("qrc:/SCU_Nexus/qml/App.qml");
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
                         if (!obj && url == objUrl)
                             QCoreApplication::exit(-1);
                     }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
