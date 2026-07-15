#include <QtTest>

#include <QDirIterator>
#include <QFile>

namespace {

QString sourcePath(const QString &relativePath)
{
    return QStringLiteral(SCU_NEXUS_SOURCE_DIR "/") + relativePath;
}

QString readUtf8(const QString &relativePath)
{
    QFile file(sourcePath(relativePath));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

} // namespace

class UiContractsTests final : public QObject
{
    Q_OBJECT

private slots:
    void qmlNeverEmbedsWebRuntime()
    {
        QDirIterator files(sourcePath(QStringLiteral("qml")),
                           QStringList{QStringLiteral("*.qml")},
                           QDir::Files,
                           QDirIterator::Subdirectories);

        while (files.hasNext()) {
            const QString path = files.next();
            QFile file(path);
            QVERIFY2(file.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(path));
            const QString source = QString::fromUtf8(file.readAll());

            QVERIFY2(!source.contains(QStringLiteral("WebView")), qPrintable(path));
            QVERIFY2(!source.contains(QStringLiteral("WebEngine")), qPrintable(path));
            QVERIFY2(!source.contains(QStringLiteral("QtWebChannel")), qPrintable(path));
        }
    }

    void shellUsesDedicatedResponsiveNavigation()
    {
        const QString shell = readUtf8(QStringLiteral("qml/MainShell.qml"));
        const QString navigation = readUtf8(QStringLiteral("qml/AppNavigation.qml"));

        QVERIFY(!shell.isEmpty());
        QVERIFY2(!navigation.isEmpty(), "AppNavigation.qml must exist");
        QVERIFY(shell.contains(QStringLiteral("AppNavigation")));
        QVERIFY(navigation.contains(QStringLiteral("compact")));
        QVERIFY(navigation.contains(QStringLiteral("router.navigate")));

        const QStringList routes{
            QStringLiteral("Schedule"),
            QStringLiteral("AcademicCalendar"),
            QStringLiteral("Classroom"),
            QStringLiteral("Grades"),
            QStringLiteral("Settings")};
        for (const QString &route : routes) {
            QVERIFY2(navigation.contains(route.toUtf8()), qPrintable(route));
        }
    }

    void shellPreservesProtectedPageStates()
    {
        const QString shell = readUtf8(QStringLiteral("qml/MainShell.qml"));

        QVERIFY(shell.contains(QStringLiteral("LoginRequiredView")));
        QVERIFY(shell.contains(QStringLiteral("protectedRouteBlocked")));
        QVERIFY(shell.contains(QStringLiteral("Classroom")));
        QVERIFY(shell.contains(QStringLiteral("Grades")));
    }

    void scheduleKeepsDistinctActionsAndDataDrivenGeometry()
    {
        const QString page = readUtf8(QStringLiteral("qml/pages/schedule/SchedulePage.qml"));
        const QString grid = readUtf8(QStringLiteral("qml/components/schedule/CourseGrid.qml"));

        QVERIFY(page.contains(QStringLiteral("课表管理")));
        QVERIFY(page.contains(QStringLiteral("课表设置")));
        QVERIFY(page.contains(QStringLiteral("goPreviousWeek")));
        QVERIFY(page.contains(QStringLiteral("goNextWeek")));
        QVERIFY(page.contains(QStringLiteral("goToCurrentWeek")));
        QVERIFY(page.contains(QStringLiteral("syncCurrentWeek")));

        QVERIFY(grid.contains(QStringLiteral("dayOfWeek - 1")));
        QVERIFY(grid.contains(QStringLiteral("startSection - 1")));
        QVERIFY(grid.contains(QStringLiteral("endSection - startSection + 1")));
        QVERIFY(grid.contains(QStringLiteral("totalTracks")));
    }

    void sourceAttributionsExposeExactClickableUrls()
    {
        const QString component = readUtf8(
            QStringLiteral("qml/components/SourceAttribution.qml"));
        const QString schedule = readUtf8(
            QStringLiteral("qml/pages/schedule/SchedulePage.qml"));
        const QString calendar = readUtf8(
            QStringLiteral("qml/pages/calendar/AcademicCalendarPage.qml"));
        const QString classroom = readUtf8(
            QStringLiteral("qml/pages/classroom/ClassroomPage.qml"));

        QVERIFY2(!component.isEmpty(), "SourceAttribution.qml must exist");
        QVERIFY(component.contains(QStringLiteral("数据来自")));
        QVERIFY(component.contains(QStringLiteral("Qt.openUrlExternally")));
        QVERIFY(component.contains(QStringLiteral("MouseArea")));
        QVERIFY(schedule.contains(QStringLiteral(
            "http://zhjw.scu.edu.cn/student/courseSelect/thisSemesterCurriculum/index")));
        QVERIFY(calendar.contains(QStringLiteral("https://jwc.scu.edu.cn/cdxl.htm")));
        QVERIFY(classroom.contains(QStringLiteral(
            "http://zhjw.scu.edu.cn/student/teachingResources/classroomCurriculum/index")));
    }

    void scheduleDialogsAreResponsiveAndPreserveBusinessSignals()
    {
        const QString management = readUtf8(
            QStringLiteral("qml/pages/schedule/ScheduleManagementDialog.qml"));
        const QString settings = readUtf8(
            QStringLiteral("qml/pages/schedule/ScheduleSettingsDialog.qml"));

        QVERIFY(management.contains(QStringLiteral("width: Math.min")));
        QVERIFY(management.contains(QStringLiteral("switchSchedule")));
        QVERIFY(management.contains(QStringLiteral("newSchedule")));
        QVERIFY(management.contains(QStringLiteral("renameSchedule")));
        QVERIFY(management.contains(QStringLiteral("deleteScheduleRequested")));

        QVERIFY(settings.contains(QStringLiteral("width: Math.min")));
        QVERIFY(settings.contains(QStringLiteral("height: Math.min")));
        QVERIFY(settings.contains(QStringLiteral("saveClicked")));
        QVERIFY(settings.contains(QStringLiteral("timeSlotPreset")));
        QVERIFY(settings.contains(QStringLiteral("autoSyncTime")));
        QVERIFY(settings.contains(QStringLiteral("timeSlots")));
    }

    void calendarUsesRealResponsiveImagesAndCompleteStates()
    {
        const QString page = readUtf8(
            QStringLiteral("qml/pages/calendar/AcademicCalendarPage.qml"));

        QVERIFY(page.contains(QStringLiteral("academicCalendarViewModel.imageUrls")));
        QVERIFY(page.contains(QStringLiteral("Image.PreserveAspectFit")));
        QVERIFY(page.contains(QStringLiteral("sourceSize")));
        QVERIFY(page.contains(QStringLiteral("anchors.horizontalCenter")));
        QVERIFY(page.contains(QStringLiteral("Image.Loading")));
        QVERIFY(page.contains(QStringLiteral("Image.Error")));
        QVERIFY(page.contains(QStringLiteral("QueryStatePane")));
        QVERIFY(page.contains(QStringLiteral("ScrollView")));
    }

    void classroomPagePreservesMigratedQueryFlowAndAllStates()
    {
        const QString page = readUtf8(
            QStringLiteral("qml/pages/classroom/ClassroomPage.qml"));
        const QString card = readUtf8(
            QStringLiteral("qml/pages/classroom/ClassroomRoomCard.qml"));
        const QString detail = readUtf8(
            QStringLiteral("qml/pages/classroom/ClassroomDetailDialog.qml"));
        const QString dateDialog = readUtf8(
            QStringLiteral("qml/pages/classroom/ClassroomDateDialog.qml"));
        const QString periodDialog = readUtf8(
            QStringLiteral("qml/pages/classroom/ClassroomPeriodDialog.qml"));
        const QString navigation = readUtf8(QStringLiteral("qml/AppNavigation.qml"));
        const QString routerHeader = readUtf8(QStringLiteral("src/app/Router.h"));
        const QString routerSource = readUtf8(QStringLiteral("src/app/Router.cpp"));

        QVERIFY2(!page.isEmpty(), "ClassroomPage.qml must exist");
        QVERIFY(navigation.contains(QStringLiteral("教室查询")));
        QVERIFY(!navigation.contains(QStringLiteral("route: \"ExamPlan\"")));
        QVERIFY(routerHeader.contains(QStringLiteral("Classroom")));
        QVERIFY(routerSource.contains(QStringLiteral("教室查询")));

        QVERIFY(page.contains(QStringLiteral("classroomViewModel.campuses")));
        QVERIFY(page.contains(QStringLiteral("classroomViewModel.buildings")));
        QVERIFY(page.contains(QStringLiteral("classroomViewModel.rooms")));
        QVERIFY(page.contains(QStringLiteral("selectedDate")));
        QVERIFY(page.contains(QStringLiteral("setPeriodFilter")));
        QVERIFY(page.contains(QStringLiteral("QueryStatePane")));
        QVERIFY(page.contains(QStringLiteral("loginMessage")));

        QVERIFY(card.contains(QStringLiteral("Card {")));
        QVERIFY(card.contains(QStringLiteral("periods")));
        QVERIFY(card.contains(QStringLiteral("statusText")));
        QVERIFY(card.contains(QStringLiteral("12")));
        QVERIFY(detail.contains(QStringLiteral("座位数")));
        QVERIFY(detail.contains(QStringLiteral("可借用")));
        QVERIFY(detail.contains(QStringLiteral("备注")));
        QVERIFY(detail.contains(QStringLiteral("第")));
        QVERIFY(dateDialog.contains(QStringLiteral("selectedDate")));
        QVERIFY(periodDialog.contains(QStringLiteral("periodStart")));
        QVERIFY(periodDialog.contains(QStringLiteral("periodEnd")));
    }

    void gradesExposeThreeRealFilteredViewsWithoutRanking()
    {
        const QString page = readUtf8(
            QStringLiteral("qml/pages/grades/GradesPage.qml"));
        const QString custom = readUtf8(
            QStringLiteral("qml/pages/grades/CustomStatsTab.qml"));

        QVERIFY(page.contains(QStringLiteral("方案成绩")));
        QVERIFY(page.contains(QStringLiteral("及格成绩")));
        QVERIFY(page.contains(QStringLiteral("自定义统计")));
        QVERIFY(page.contains(QStringLiteral("setSearchQuery")));
        QVERIFY(page.contains(QStringLiteral("setSelectedTerm")));
        QVERIFY(page.contains(QStringLiteral("setSelectedCourseType")));
        QVERIFY(page.contains(QStringLiteral("refreshSchemeScores")));
        QVERIFY(page.contains(QStringLiteral("refreshPassingScores")));
        QVERIFY(custom.contains(QStringLiteral("customStatsForSelected")));

        QDirIterator files(sourcePath(QStringLiteral("qml/pages/grades")),
                           QStringList{QStringLiteral("*.qml")},
                           QDir::Files);
        while (files.hasNext()) {
            const QString path = files.next();
            QFile file(path);
            QVERIFY2(file.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(path));
            const QString source = QString::fromUtf8(file.readAll());
            QVERIFY2(!source.contains(QStringLiteral("排名")), qPrintable(path));
        }
    }

    void settingsExposesQwenApiKeyPersistenceControls()
    {
        const QString page = readUtf8(QStringLiteral("qml/SettingsPage.qml"));

        QVERIFY(page.contains(QStringLiteral("智能服务")));
        QVERIFY(page.contains(QStringLiteral("Qwen API Key")));
        QVERIFY(page.contains(QStringLiteral("passwordMode: true")));
        QVERIFY(page.contains(QStringLiteral("appSettings.qwenApiKey")));
        QVERIFY(page.contains(QStringLiteral("saveQwenApiKey")));
        QVERIFY(page.contains(QStringLiteral("clearQwenApiKey")));
        QVERIFY(!page.contains(QStringLiteral("仅用于演示")));
        QVERIFY(!page.contains(QStringLiteral("不会发送")));
        QVERIFY(!page.contains(QStringLiteral("模拟")));
    }
};

QTEST_APPLESS_MAIN(UiContractsTests)
#include "test_ui_contracts.moc"
