#include <QGuiApplication>
#include <QDir>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QStandardPaths>
#include <QtQuickControls2/QQuickStyle>

#include "src/app/AppController.h"
#include "src/app/Router.h"
#include "src/app/ThemeManager.h"
#include "src/app/ToastManager.h"
#include "src/repositories/QueryCacheRepository.h"
#include "src/services/api/ZhjwApiService.h"
#include "src/services/auth/ScuAuthService.h"
#include "src/services/auth/ZhjwAuthService.h"
#include "src/services/zhjw/ZhjwQueryService.h"
#include "src/viewmodels/AcademicCalendarViewModel.h"
#include "src/viewmodels/ExamPlanViewModel.h"
#include "src/viewmodels/GradesViewModel.h"

// 初始化应用运行环境并进入主事件循环。
int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    app.setOrganizationName("SCU");
    app.setApplicationName("SCU_Nexus");
    app.setApplicationVersion("0.1.0");

    // Fluent WinUI 3 是 Qt 6.8+ 自带的现代 Windows 11 风格；
    // 若运行环境未安装该样式模块，Qt 会回退到默认样式并打印告警。
    QQuickStyle::setStyle("FluentWinUI3");

    ScuAuthService scuAuthService;
    AppController appController(nullptr, &scuAuthService);
    Router router;
    ThemeManager themeManager;
    ToastManager toastManager;
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);

    QueryCacheRepository queryCache(dataDir + QStringLiteral("/query_cache.sqlite"));
    queryCache.open();

    ZhjwAuthService zhjwAuthService(nullptr, &scuAuthService);
    ZhjwApiService zhjwApiService(nullptr, &zhjwAuthService);
    ZhjwApiQueryService zhjwQueryService(nullptr, &zhjwApiService);
    zhjwQueryService.setLoggedIn(appController.loggedIn());

    AcademicCalendarViewModel academicCalendarViewModel(&queryCache);
    ExamPlanViewModel examPlanViewModel(&queryCache, &zhjwQueryService);
    GradesViewModel gradesViewModel(&queryCache, &zhjwQueryService);

    QObject::connect(&appController, &AppController::loginStateChanged,
                     &zhjwQueryService, &ZhjwApiQueryService::setLoggedIn);
    QObject::connect(&academicCalendarViewModel, &AcademicCalendarViewModel::toastRequested,
                     &toastManager, [&toastManager](const QString &message) { toastManager.show(message, QStringLiteral("warning")); });
    QObject::connect(&examPlanViewModel, &ExamPlanViewModel::toastRequested,
                     &toastManager, [&toastManager](const QString &message) { toastManager.show(message, QStringLiteral("warning")); });
    QObject::connect(&gradesViewModel, &GradesViewModel::toastRequested,
                     &toastManager, [&toastManager](const QString &message) { toastManager.show(message, QStringLiteral("warning")); });

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("appController", &appController);
    engine.rootContext()->setContextProperty("router", &router);
    engine.rootContext()->setContextProperty("themeManager", &themeManager);
    engine.rootContext()->setContextProperty("toastManager", &toastManager);
    engine.rootContext()->setContextProperty("academicCalendarViewModel", &academicCalendarViewModel);
    engine.rootContext()->setContextProperty("examPlanViewModel", &examPlanViewModel);
    engine.rootContext()->setContextProperty("gradesViewModel", &gradesViewModel);

    const QUrl url("qrc:/SCU_Nexus/qml/App.qml");
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
                         if (!obj && url == objUrl)
                             QCoreApplication::exit(-1);
                     }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
