
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QDebug>
#include "src/models/Course.h"
#include "src/models/ScheduleConfig.h"
#include "src/models/TimeSlot.h"
#include "src/repositories/ScheduleRepository.h"
#include "src/viewmodels/ScheduleViewModel.h"
#include "src/viewmodels/CourseEditViewModel.h"
#include "src/viewmodels/ScheduleImportViewModel.h"

using namespace SCUNexus;

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    app.setOrganizationName("SCU_Nexus");
    app.setApplicationName("SCU_Nexus");
    app.setApplicationVersion("1.0.0");

    // Use Basic style (Material may require additional imports)
    QQuickStyle::setStyle("Basic");

    // ---- Create core services ----
    ScheduleRepository repository;
    if (!repository.init()) {
        qWarning() << "WARNING: Failed to initialize database. Running without persistence.";
    } else {
        qDebug() << "Database initialized at:" << repository.databasePath();
    }

    // ---- Create ViewModels ----
    ScheduleViewModel scheduleVM;
    scheduleVM.setRepository(&repository);

    CourseEditViewModel courseEditVM;
    courseEditVM.setRepository(&repository);

    ScheduleImportViewModel importVM;
    importVM.setRepository(&repository);

    // Load data
    scheduleVM.load();
    qDebug() << "Schedule loaded. Has schedule:" << scheduleVM.hasSchedule()
             << "Week:" << scheduleVM.currentWeek();

    // ---- Create QML engine ----
    QQmlApplicationEngine engine;

    // Expose ViewModels to QML
    QQmlContext* context = engine.rootContext();
    context->setContextProperty("scheduleViewModel", &scheduleVM);
    context->setContextProperty("courseEditViewModel", &courseEditVM);
    context->setContextProperty("importViewModel", &importVM);

    // Load main QML from resource file
    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "FATAL: Failed to load QML UI. Check that qml.qrc is compiled in.";
        return -1;
    }

    qDebug() << "Application started successfully.";
    return app.exec();
}
