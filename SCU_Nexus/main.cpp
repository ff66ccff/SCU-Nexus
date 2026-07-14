#include <QGuiApplication>
#include <QDir>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QStandardPaths>
#include <QtQuickControls2/QQuickStyle>

#include "src/app/AppController.h"
#include "src/app/AppSettings.h"
#include "src/app/Router.h"
#include "src/app/ThemeManager.h"
#include "src/app/ToastManager.h"
#include "src/repositories/QueryCacheRepository.h"
#include "src/repositories/ScheduleRepository.h"
#include "src/services/api/ZhjwApiService.h"
#include "src/services/auth/ScuAuthService.h"
#include "src/services/auth/ZhjwAuthService.h"
#include "src/services/zhjw/ZhjwQueryService.h"
#include "src/viewmodels/AcademicCalendarViewModel.h"
#include "src/viewmodels/ExamPlanViewModel.h"
#include "src/viewmodels/GradesViewModel.h"
#include "src/viewmodels/QueryCacheViewModel.h"
#include "src/viewmodels/ScheduleViewModel.h"
#include "src/viewmodels/ScheduleImportViewModel.h"
#include "src/viewmodels/CourseEditViewModel.h"

// 初始化应用运行环境并进入主事件循环。
int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    app.setOrganizationName("SCU");
    app.setApplicationName("SCU_Nexus");
    app.setApplicationVersion(QStringLiteral(SCU_NEXUS_VERSION));

    // Fluent WinUI 3 是 Qt 6.8+ 自带的现代 Windows 11 风格；
    // 若运行环境未安装该样式模块，Qt 会回退到默认样式并打印告警。
    QQuickStyle::setStyle("FluentWinUI3");

    ScuAuthService scuAuthService;
    AppSettings appSettings;
    Router router;
    ThemeManager themeManager;
    ToastManager toastManager;
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);

    QueryCacheRepository queryCache(dataDir + QStringLiteral("/query_cache.sqlite"));
    SCUNexus::ScheduleRepository scheduleRepository;
    AppController appController(
        nullptr,
        &scuAuthService,
        [&queryCache]() {
            const bool ok = queryCache.open();
            return StartupStepResult{ok, ok ? QString{} : queryCache.lastError()};
        },
        [&scheduleRepository]() {
            const bool ok = scheduleRepository.init();
            return StartupStepResult{
                ok,
                ok ? QString{} : QStringLiteral("无法打开或初始化课表数据库")};
        });

    ZhjwAuthService zhjwAuthService(nullptr, &scuAuthService);
    ZhjwApiService zhjwApiService(nullptr, &zhjwAuthService);
    ZhjwApiQueryService zhjwQueryService(nullptr, &zhjwApiService);
    zhjwQueryService.setLoggedIn(appController.loggedIn());

    QueryCacheViewModel queryCacheViewModel(&queryCache);
    AcademicCalendarViewModel academicCalendarViewModel(&queryCache);
    ExamPlanViewModel examPlanViewModel(&queryCache, &zhjwQueryService);
    GradesViewModel gradesViewModel(&queryCache, &zhjwQueryService);

    SCUNexus::ScheduleViewModel scheduleViewModel;
    SCUNexus::ScheduleImportViewModel scheduleImportViewModel;
    SCUNexus::CourseEditViewModel courseEditViewModel;
    scheduleViewModel.setRepository(&scheduleRepository);
    scheduleImportViewModel.setRepository(&scheduleRepository);
    scheduleImportViewModel.setLoggedIn(appController.loggedIn());
    courseEditViewModel.setRepository(&scheduleRepository);

    scheduleImportViewModel.setRemoteApi(
        [&zhjwApiService](SCUNexus::ScheduleImportViewModel::SemestersResult done) {
            zhjwApiService.fetchSemesters(
                [done = std::move(done)](const QList<SemesterDto>& semesters,
                                         const ApiError& error) mutable {
                    if (!error.message.isEmpty()) {
                        done({}, error.message);
                        return;
                    }
                    QVariantList result;
                    for (const SemesterDto& semester : semesters) {
                        QVariantMap item;
                        item["planCode"] = semester.value;
                        item["label"] = semester.label;
                        item["isCurrent"] = semester.label.contains(QStringLiteral("（当前）"))
                            || semester.label.contains(QStringLiteral("(当前)"));
                        result.append(item);
                    }
                    done(result, {});
                });
        },
        [&zhjwApiService](const QString& planCode,
                          SCUNexus::ScheduleImportViewModel::ScheduleResult done) {
            zhjwApiService.fetchJwxtSchedule(
                planCode,
                [done = std::move(done)](const QJsonObject& schedule,
                                         const ApiError& error) mutable {
                    done(schedule, error.message);
                });
        },
        [&zhjwApiService](SCUNexus::ScheduleImportViewModel::WeekResult done) {
            zhjwApiService.fetchCurrentWeek(
                [done = std::move(done)](int week, const ApiError& error) mutable {
                    done(week, error.message);
                });
        });

    QObject::connect(&appController, &AppController::loginStateChanged,
                     &zhjwQueryService, &ZhjwApiQueryService::setLoggedIn);
    QObject::connect(&appController, &AppController::loginStateChanged,
                     &scheduleImportViewModel, &SCUNexus::ScheduleImportViewModel::setLoggedIn);
    QObject::connect(&academicCalendarViewModel, &AcademicCalendarViewModel::toastRequested,
                     &toastManager, [&toastManager](const QString &message) { toastManager.show(message, QStringLiteral("warning")); });
    QObject::connect(&examPlanViewModel, &ExamPlanViewModel::toastRequested,
                     &toastManager, [&toastManager](const QString &message) { toastManager.show(message, QStringLiteral("warning")); });
    QObject::connect(&gradesViewModel, &GradesViewModel::toastRequested,
                     &toastManager, [&toastManager](const QString &message) { toastManager.show(message, QStringLiteral("warning")); });
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("appController", &appController);
    engine.rootContext()->setContextProperty("appSettings", &appSettings);
    engine.rootContext()->setContextProperty("router", &router);
    engine.rootContext()->setContextProperty("themeManager", &themeManager);
    engine.rootContext()->setContextProperty("toastManager", &toastManager);
    engine.rootContext()->setContextProperty("queryCacheViewModel", &queryCacheViewModel);
    engine.rootContext()->setContextProperty("academicCalendarViewModel", &academicCalendarViewModel);
    engine.rootContext()->setContextProperty("examPlanViewModel", &examPlanViewModel);
    engine.rootContext()->setContextProperty("gradesViewModel", &gradesViewModel);
    engine.rootContext()->setContextProperty("scheduleViewModel", &scheduleViewModel);
    engine.rootContext()->setContextProperty("scheduleImportViewModel", &scheduleImportViewModel);
    engine.rootContext()->setContextProperty("courseEditViewModel", &courseEditViewModel);

    const QUrl url("qrc:/SCU_Nexus/qml/App.qml");
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
                         if (!obj && url == objUrl)
                             QCoreApplication::exit(-1);
                     }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
