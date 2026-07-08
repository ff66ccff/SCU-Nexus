#include "repositories/QueryCacheRepository.h"
#include "services/zhjw/MockZhjwApiService.h"
#include "viewmodels/AcademicCalendarViewModel.h"
#include "viewmodels/ExamPlanViewModel.h"
#include "viewmodels/GradesViewModel.h"
#include "widgets/AppStyle.h"
#include "widgets/MainWindow.h"

#include <QApplication>
#include <QDir>
#include <QStandardPaths>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    AppStyle::apply(&app);

    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);

    QueryCacheRepository cache(dataDir + QStringLiteral("/query_cache.sqlite"));
    cache.open();

    MockZhjwApiService zhjw;
    zhjw.setLoggedIn(true);

    AcademicCalendarViewModel calendarViewModel(&cache);
    ExamPlanViewModel examViewModel(&cache, &zhjw);
    GradesViewModel gradesViewModel(&cache, &zhjw);

    MainWindow window(&calendarViewModel, &examViewModel, &gradesViewModel);
    window.show();
    window.loadAll();

    return app.exec();
}
