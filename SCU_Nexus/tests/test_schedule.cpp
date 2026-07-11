#include <QtTest>
#include <QTemporaryDir>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlQuery>
#include "../src/models/Course.h"
#include "../src/models/ScheduleConfig.h"
#include "../src/models/TimeSlot.h"
#include "../src/repositories/ScheduleRepository.h"
#include "../src/services/course/JwxtScheduleParser.h"
#include "../src/services/course/ScheduleImportService.h"
#include "../src/services/course/CourseLayoutService.h"
#include "../src/viewmodels/ScheduleViewModel.h"
#include "../src/viewmodels/CourseEditViewModel.h"
#include "../src/viewmodels/ScheduleImportViewModel.h"

using namespace SCUNexus;

class TestCourseModel : public QObject {
    Q_OBJECT

private slots:
    // ========== Course Tests ==========

    void testCourseIsInWeekRange() {
        Course c;
        c.startWeek = 3;
        c.endWeek = 10;

        QVERIFY(c.isInWeekRange(3));
        QVERIFY(c.isInWeekRange(5));
        QVERIFY(c.isInWeekRange(10));
        QVERIFY(!c.isInWeekRange(2));
        QVERIFY(!c.isInWeekRange(11));
    }

    void testCourseIsActiveInWeek_Every() {
        Course c;
        c.startWeek = 1;
        c.endWeek = 10;
        c.weekType = WeekType::Every;

        QVERIFY(c.isActiveInWeek(1));
        QVERIFY(c.isActiveInWeek(2));
        QVERIFY(c.isActiveInWeek(3));
        QVERIFY(!c.isActiveInWeek(11));
    }

    void testCourseIsActiveInWeek_Odd() {
        Course c;
        c.startWeek = 1;
        c.endWeek = 10;
        c.weekType = WeekType::Odd;

        QVERIFY(c.isActiveInWeek(1));
        QVERIFY(!c.isActiveInWeek(2));
        QVERIFY(c.isActiveInWeek(3));
        QVERIFY(!c.isActiveInWeek(4));
    }

    void testCourseIsActiveInWeek_Even() {
        Course c;
        c.startWeek = 1;
        c.endWeek = 10;
        c.weekType = WeekType::Even;

        QVERIFY(!c.isActiveInWeek(1));
        QVERIFY(c.isActiveInWeek(2));
        QVERIFY(!c.isActiveInWeek(3));
        QVERIFY(c.isActiveInWeek(4));
    }

    void testCourseConflictsWith_SameSlot() {
        Course a;
        a.id = "a";
        a.dayOfWeek = 3;
        a.startSection = 3;
        a.endSection = 4;
        a.startWeek = 1;
        a.endWeek = 10;
        a.weekType = WeekType::Every;

        Course b;
        b.id = "b";
        b.dayOfWeek = 3;
        b.startSection = 3;
        b.endSection = 4;
        b.startWeek = 1;
        b.endWeek = 10;
        b.weekType = WeekType::Every;

        QVERIFY(a.conflictsWith(b));
    }

    void testCourseConflictsWith_DifferentDay() {
        Course a;
        a.id = "a";
        a.dayOfWeek = 3;
        a.startSection = 3;
        a.endSection = 4;
        a.startWeek = 1;
        a.endWeek = 10;
        a.weekType = WeekType::Every;

        Course b;
        b.id = "b";
        b.dayOfWeek = 4;
        b.startSection = 3;
        b.endSection = 4;
        b.startWeek = 1;
        b.endWeek = 10;
        b.weekType = WeekType::Every;

        QVERIFY(!a.conflictsWith(b));
    }

    void testCourseConflictsWith_NoSectionOverlap() {
        Course a;
        a.id = "a";
        a.dayOfWeek = 3;
        a.startSection = 1;
        a.endSection = 2;
        a.startWeek = 1;
        a.endWeek = 10;
        a.weekType = WeekType::Every;

        Course b;
        b.id = "b";
        b.dayOfWeek = 3;
        b.startSection = 3;
        b.endSection = 4;
        b.startWeek = 1;
        b.endWeek = 10;
        b.weekType = WeekType::Every;

        QVERIFY(!a.conflictsWith(b));
    }

    void testCourseConflictsWith_NoWeekOverlap() {
        Course a;
        a.id = "a";
        a.dayOfWeek = 3;
        a.startSection = 3;
        a.endSection = 4;
        a.startWeek = 1;
        a.endWeek = 5;
        a.weekType = WeekType::Every;

        Course b;
        b.id = "b";
        b.dayOfWeek = 3;
        b.startSection = 3;
        b.endSection = 4;
        b.startWeek = 6;
        b.endWeek = 10;
        b.weekType = WeekType::Every;

        QVERIFY(!a.conflictsWith(b));
    }

    void testCourseConflictsWith_OddEvenMutualExclusion() {
        Course a;
        a.id = "a";
        a.dayOfWeek = 3;
        a.startSection = 3;
        a.endSection = 4;
        a.startWeek = 1;
        a.endWeek = 10;
        a.weekType = WeekType::Odd;

        Course b;
        b.id = "b";
        b.dayOfWeek = 3;
        b.startSection = 3;
        b.endSection = 4;
        b.startWeek = 1;
        b.endWeek = 10;
        b.weekType = WeekType::Even;

        QVERIFY(!a.conflictsWith(b));
    }

    void testCourseConflictsWith_PartialWeekOverlap() {
        // Odd course weeks 1-5, Every course weeks 3-10
        // Should conflict on weeks 3, 5
        Course a;
        a.id = "a";
        a.dayOfWeek = 3;
        a.startSection = 3;
        a.endSection = 4;
        a.startWeek = 1;
        a.endWeek = 5;
        a.weekType = WeekType::Odd;

        Course b;
        b.id = "b";
        b.dayOfWeek = 3;
        b.startSection = 3;
        b.endSection = 4;
        b.startWeek = 3;
        b.endWeek = 10;
        b.weekType = WeekType::Every;

        QVERIFY(a.conflictsWith(b));
    }

    void testCourseCopyWithPreservesUnspecifiedFields() {
        Course original;
        original.id = "course";
        original.name = "高等数学";
        original.teacher = "张老师";
        original.location = "一教A101";
        original.startWeek = 1;

        const Course copied = original.copyWith({}, {}, {}, {}, 3);
        QCOMPARE(copied.id, original.id);
        QCOMPARE(copied.name, original.name);
        QCOMPARE(copied.teacher, original.teacher);
        QCOMPARE(copied.location, original.location);
        QCOMPARE(copied.startWeek, 3);
    }

    // ========== ScheduleConfig Tests ==========

    void testScheduleConfigCurrentWeek() {
        ScheduleConfig config;
        config.semesterStartDate = QDate(2026, 2, 23); // A Monday
        config.totalWeeks = 20;

        // Week 1
        QCOMPARE(config.currentWeek(QDate(2026, 2, 23)), 1);
        QCOMPARE(config.currentWeek(QDate(2026, 2, 25)), 1);

        // Week 2
        QCOMPARE(config.currentWeek(QDate(2026, 3, 2)), 2);

        // Before semester
        QCOMPARE(config.currentWeek(QDate(2026, 2, 20)), 1);

        // After semester
        QCOMPARE(config.currentWeek(QDate(2026, 8, 1)), 20);
    }

    void testScheduleConfigReadsLegacyEndDate()
    {
        const auto config = ScheduleConfig::fromJson({
            {"semesterStartDate", "2026-02-23"},
            {"semesterEndDate", "2026-06-28"}
        });
        QCOMPARE(config.totalWeeks, 18);
    }

    void testScheduleConfigSplitsLegacySectionsPerDay()
    {
        const auto config = ScheduleConfig::fromJson({{"sectionsPerDay", 10}});
        QCOMPARE(config.morningSections, 4);
        QCOMPARE(config.afternoonSections, 5);
        QCOMPARE(config.eveningSections, 1);
    }

    void testScheduleConfigCreatesMissingTimeSlots()
    {
        const auto config = ScheduleConfig::fromJson({
            {"morningSections", 2}, {"afternoonSections", 1}, {"eveningSections", 1},
            {"courseDuration", 45}, {"breakDuration", 10}
        });
        QCOMPARE(config.timeSlots.size(), 4);
        QCOMPARE(config.timeSlots.at(0).startTime, QTime(8, 0));
        QCOMPARE(config.timeSlots.at(2).startTime, QTime(14, 0));
        QCOMPARE(config.timeSlots.at(3).startTime, QTime(19, 0));
    }

    void testJiangAnTimeSlots() {
        const auto timeSlots = ScheduleConfig::jiangAnTimeSlots();
        QCOMPARE(timeSlots.size(), 12);

        // Check key time points
        QCOMPARE(timeSlots[0].startTime, QTime(8, 15));
        QCOMPARE(timeSlots[0].endTime, QTime(9, 0));
        QCOMPARE(timeSlots[3].startTime, QTime(11, 10));
        QCOMPARE(timeSlots[4].startTime, QTime(13, 50)); // Afternoon start
        QCOMPARE(timeSlots[9].startTime, QTime(19, 20)); // Evening start
    }

    void testWangJiangHuaXiTimeSlots() {
        const auto timeSlots = ScheduleConfig::wangJiangHuaXiTimeSlots();
        QCOMPARE(timeSlots.size(), 12);

        QCOMPARE(timeSlots[0].startTime, QTime(8, 0));
        QCOMPARE(timeSlots[3].startTime, QTime(10, 55));
        QCOMPARE(timeSlots[4].startTime, QTime(14, 0));
        QCOMPARE(timeSlots[9].startTime, QTime(19, 30));
    }

    void testTimeSlotsForCampusName() {
        auto jiangAn = ScheduleConfig::timeSlotsForCampusName("江安校区");
        QVERIFY(jiangAn.has_value());
        QCOMPARE(jiangAn->size(), 12);

        auto wangJiang = ScheduleConfig::timeSlotsForCampusName("望江校区");
        QVERIFY(wangJiang.has_value());
        QCOMPARE(wangJiang->size(), 12);

        auto huaXi = ScheduleConfig::timeSlotsForCampusName("华西校区");
        QVERIFY(huaXi.has_value());

        auto unknown = ScheduleConfig::timeSlotsForCampusName("未知校区");
        QVERIFY(!unknown.has_value());
    }

    // ========== TimeSlot Tests ==========

    void testTimeSlotValidation() {
        TimeSlot valid{QTime(8, 0), QTime(9, 0)};
        QVERIFY(valid.isValid());

        TimeSlot invalid{QTime(9, 0), QTime(8, 0)};
        QVERIFY(!invalid.isValid());
    }

    void testTimeSlotJsonRoundtrip() {
        TimeSlot original{QTime(8, 15), QTime(9, 0)};
        QJsonObject json = original.toJson();
        TimeSlot restored = TimeSlot::fromJson(json);
        QCOMPARE(restored.startTime, original.startTime);
        QCOMPARE(restored.endTime, original.endTime);
    }

    // ========== JwxtScheduleParser Tests ==========

    void testJwxtParseContinuousWeeks() {
        auto result = JwxtScheduleParser::parseClassWeek("111111111111111100000000");
        QVERIFY(result.has_value());
        QCOMPARE(result->startWeek, 1);
        QCOMPARE(result->endWeek, 16);
        QCOMPARE(result->weekType, WeekType::Every);
    }

    void testJwxtParseOddWeeks() {
        // Weeks 1,3,5,7,9,11,13,15 are active
        QString bitStr;
        for (int i = 1; i <= 20; i++) {
            bitStr += (i % 2 == 1) ? '1' : '0';
        }
        auto result = JwxtScheduleParser::parseClassWeek(bitStr);
        QVERIFY(result.has_value());
        QCOMPARE(result->weekType, WeekType::Odd);
    }

    void testJwxtParseEvenWeeks() {
        // Weeks 2,4,6,8,10 are active
        QString bitStr;
        for (int i = 1; i <= 10; i++) {
            bitStr += (i % 2 == 0) ? '1' : '0';
        }
        auto result = JwxtScheduleParser::parseClassWeek(bitStr);
        QVERIFY(result.has_value());
        QCOMPARE(result->weekType, WeekType::Even);
    }

    void testJwxtParseEmptyClassWeek() {
        auto result = JwxtScheduleParser::parseClassWeek("000000000000000000000000");
        QVERIFY(!result.has_value());
    }

    void testJwxtRejectsMixedValidAndMalformedEntries() {
        const QJsonObject validTime{
            {"classDay", 1}, {"classSessions", 1}, {"continuingSession", 2},
            {"classWeek", "11111111111111111111"}
        };
        const QJsonObject malformedTime{
            {"classDay", 9}, {"classSessions", 1}, {"continuingSession", 2},
            {"classWeek", "11111111111111111111"}
        };
        const QJsonObject detail{
            {"courseName", "高等数学"},
            {"timeAndPlaceList", QJsonArray{validTime, malformedTime}}
        };

        const auto result = JwxtScheduleParser::parse(
            QJsonObject{{"xkxx", QJsonArray{QJsonObject{{"course", detail}}}}});
        QVERIFY(!result.success);
        QVERIFY(result.courses.isEmpty());
        QVERIFY(result.errorMessage.contains("导入数据格式异常"));
    }

    void testJwxtRejectsEmptyOriginalCourseName() {
        const QJsonObject time{
            {"classDay", 1}, {"classSessions", 1}, {"continuingSession", 2},
            {"classWeek", "11111111111111111111"}
        };
        const QJsonObject detail{
            {"courseName", ""},
            {"id", QJsonObject{{"coureSequenceNumber", "01"}}},
            {"timeAndPlaceList", QJsonArray{time}}
        };

        const auto result = JwxtScheduleParser::parse(
            QJsonObject{{"xkxx", QJsonArray{QJsonObject{{"course", detail}}}}});
        QVERIFY(!result.success);
        QVERIFY(result.courses.isEmpty());
    }

    void testCleanSemesterLabel() {
        QCOMPARE(JwxtScheduleParser::cleanSemesterLabel("2025-2026学年第二学期（当前）"),
                 QString("2025-2026学年第二学期"));
        QCOMPARE(JwxtScheduleParser::cleanSemesterLabel("2025-2026学年第二学期(当前)"),
                 QString("2025-2026学年第二学期"));
        QCOMPARE(JwxtScheduleParser::cleanSemesterLabel("  有空格  "),
                 QString("有空格"));
    }

    // ========== Repository Tests ==========

    void testRepositoryInitEmpty() {
        ScheduleRepository repo;
        repo.setDatabasePath(":memory:"); // Use in-memory DB for testing
        QVERIFY(repo.init());
        QVERIFY(repo.currentScheduleId().isEmpty());
        QVERIFY(repo.allSchedules().isEmpty());
        QVERIFY(repo.currentCourses().isEmpty());
    }

    void testRepositoryAddSchedule() {
        ScheduleRepository repo;
        repo.setDatabasePath(":memory:");
        repo.init();

        ScheduleConfig config = ScheduleConfig::createDefault("测试课表");
        QVERIFY(repo.addSchedule(config));
        QCOMPARE(repo.currentScheduleId(), config.id);
        QCOMPARE(repo.allSchedules().size(), 1);
    }

    void testRepositoryFirstScheduleRollsBackWhenMetadataWriteFails() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath("add-rollback.sqlite");

        ScheduleRepository repo;
        repo.setDatabasePath(path);
        QVERIFY(repo.init());

        const QString connectionName = "add_rollback_test";
        {
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
            db.setDatabaseName(path);
            QVERIFY(db.open());
            QSqlQuery query(db);
            QVERIFY(query.exec(
                "CREATE TRIGGER reject_first_schedule_metadata "
                "BEFORE INSERT ON metadata WHEN NEW.key = 'currentScheduleId' "
                "BEGIN SELECT RAISE(ABORT, 'blocked'); END"));

            const ScheduleConfig config = ScheduleConfig::createDefault("不应残留");
            QVERIFY(!repo.addSchedule(config));
            QVERIFY(repo.currentScheduleId().isEmpty());
            QVERIFY(repo.allSchedules().isEmpty());
            QVERIFY(repo.coursesForSchedule(config.id).isEmpty());
            db.close();
        }
        QSqlDatabase::removeDatabase(connectionName);
    }

    void testRepositorySwitchSchedule() {
        ScheduleRepository repo;
        repo.setDatabasePath(":memory:");
        repo.init();

        ScheduleConfig c1 = ScheduleConfig::createDefault("课表1");
        ScheduleConfig c2 = ScheduleConfig::createDefault("课表2");
        repo.addSchedule(c1);
        repo.addSchedule(c2);

        QVERIFY(repo.switchSchedule(c2.id));
        QCOMPARE(repo.currentScheduleId(), c2.id);
    }

    void testRepositoryAddCourse() {
        ScheduleRepository repo;
        repo.setDatabasePath(":memory:");
        repo.init();

        ScheduleConfig config = ScheduleConfig::createDefault("测试");
        repo.addSchedule(config);

        Course course;
        course.id = "test-course-1";
        course.name = "高等数学";
        course.dayOfWeek = 1;
        course.startSection = 1;
        course.endSection = 2;

        QVERIFY(repo.addCourse(course));
        QCOMPARE(repo.currentCourses().size(), 1);
    }

    void testRepositoryDeleteCourse() {
        ScheduleRepository repo;
        repo.setDatabasePath(":memory:");
        repo.init();

        ScheduleConfig config = ScheduleConfig::createDefault("测试");
        repo.addSchedule(config);

        Course course;
        course.id = "test-del-1";
        course.name = "测试课程";
        repo.addCourse(course);

        QVERIFY(repo.deleteCourse("test-del-1"));
        QCOMPARE(repo.currentCourses().size(), 0);
    }

    void testRepositoryUpdateCourse() {
        ScheduleRepository repo;
        repo.setDatabasePath(":memory:");
        QVERIFY(repo.init());
        QVERIFY(repo.addSchedule(ScheduleConfig::createDefault("update")));

        Course course;
        course.id = "update-course";
        course.name = "旧名称";
        QVERIFY(repo.addCourse(course));

        course.name = "新名称";
        course.location = "一教A101";
        QVERIFY(repo.updateCourse(course));
        QCOMPARE(repo.currentCourses().first().name, QString("新名称"));
        QCOMPARE(repo.coursesForSchedule(repo.currentScheduleId()).first().location,
                 QString("一教A101"));
    }

    void testRepositoryReplaceScheduleCourses() {
        ScheduleRepository repo;
        repo.setDatabasePath(":memory:");
        repo.init();

        ScheduleConfig config = ScheduleConfig::createDefault("测试");
        repo.addSchedule(config);

        QList<Course> courses;
        for (int i = 0; i < 5; i++) {
            Course c;
            c.id = QString("rc-%1").arg(i);
            c.name = QString("课程%1").arg(i);
            courses.append(c);
        }

        QVERIFY(repo.replaceScheduleCourses(config.id, courses));
        QCOMPARE(repo.currentCourses().size(), 5);
    }

    void testRepositoryDeleteSchedule() {
        ScheduleRepository repo;
        repo.setDatabasePath(":memory:");
        repo.init();

        ScheduleConfig c1 = ScheduleConfig::createDefault("课表1");
        ScheduleConfig c2 = ScheduleConfig::createDefault("课表2");
        repo.addSchedule(c1);
        repo.addSchedule(c2);

        // Delete current schedule
        QVERIFY(repo.deleteSchedule(c1.id));
        QCOMPARE(repo.currentScheduleId(), c2.id); // Switched to remaining

        // Delete remaining schedule
        QVERIFY(repo.deleteSchedule(c2.id));
        QVERIFY(repo.currentScheduleId().isEmpty()); // No schedules left
    }

    void testRepositoryDeleteCurrentScheduleRollsBackWhenMetadataWriteFails() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath("delete-rollback.sqlite");

        ScheduleRepository repo;
        repo.setDatabasePath(path);
        QVERIFY(repo.init());
        const ScheduleConfig current = ScheduleConfig::createDefault("当前课表");
        const ScheduleConfig replacement = ScheduleConfig::createDefault("备用课表");
        QVERIFY(repo.addSchedule(current));
        QVERIFY(repo.addSchedule(replacement));

        const QString connectionName = "delete_rollback_test";
        {
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
            db.setDatabaseName(path);
            QVERIFY(db.open());
            QSqlQuery query(db);
            QVERIFY(query.exec(
                "CREATE TRIGGER reject_current_schedule_metadata "
                "BEFORE INSERT ON metadata WHEN NEW.key = 'currentScheduleId' "
                "BEGIN SELECT RAISE(ABORT, 'blocked'); END"));
            QVERIFY(!repo.deleteSchedule(current.id));
            QCOMPARE(repo.currentScheduleId(), current.id);
            QCOMPARE(repo.allSchedules().size(), 2);
            db.close();
        }
        QSqlDatabase::removeDatabase(connectionName);
    }

    void testRepositoryCurrentCourseOperationsRejectForeignScheduleIds() {
        ScheduleRepository repo;
        repo.setDatabasePath(":memory:");
        QVERIFY(repo.init());
        const ScheduleConfig first = ScheduleConfig::createDefault("第一课表");
        const ScheduleConfig second = ScheduleConfig::createDefault("第二课表");
        QVERIFY(repo.addSchedule(first));
        Course firstCourse;
        firstCourse.id = "first-course";
        firstCourse.name = "第一课程";
        QVERIFY(repo.addCourse(firstCourse));

        QVERIFY(repo.addSchedule(second));
        QVERIFY(repo.switchSchedule(second.id));
        Course foreignUpdate = firstCourse;
        foreignUpdate.name = "不应被修改";
        QVERIFY(!repo.updateCourse(foreignUpdate));
        QVERIFY(!repo.deleteCourse(firstCourse.id));
        QCOMPARE(repo.coursesForSchedule(first.id).first().name, QString("第一课程"));
    }

    void testRepositoryAtomicImportRollsBackWhenSwitchFails() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath("import-rollback.sqlite");

        ScheduleRepository repo;
        repo.setDatabasePath(path);
        QVERIFY(repo.init());
        const ScheduleConfig current = ScheduleConfig::createDefault("当前课表");
        QVERIFY(repo.addSchedule(current));

        const QString connectionName = "import_rollback_test";
        {
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
            db.setDatabaseName(path);
            QVERIFY(db.open());
            QSqlQuery query(db);
            QVERIFY(query.exec(
                "CREATE TRIGGER reject_import_switch "
                "BEFORE INSERT ON metadata WHEN NEW.key = 'currentScheduleId' "
                "BEGIN SELECT RAISE(ABORT, 'blocked'); END"));

            const ScheduleConfig imported = ScheduleConfig::createDefault("导入课表");
            Course course;
            course.id = "imported-course";
            course.name = "导入课程";
            QVERIFY(!repo.addScheduleWithCoursesAndSwitch(imported, {course}));
            QCOMPARE(repo.currentScheduleId(), current.id);
            QCOMPARE(repo.allSchedules().size(), 1);
            QVERIFY(repo.coursesForSchedule(imported.id).isEmpty());
            db.close();
        }
        QSqlDatabase::removeDatabase(connectionName);
    }

    void testRepositoryClearAllData() {
        ScheduleRepository repo;
        repo.setDatabasePath(":memory:");
        repo.init();

        ScheduleConfig config = ScheduleConfig::createDefault("测试");
        repo.addSchedule(config);

        Course course;
        course.id = "clear-1";
        course.name = "测试";
        repo.addCourse(course);

        QVERIFY(repo.clearAllCourseData());
        QVERIFY(repo.allSchedules().isEmpty());
        QVERIFY(repo.currentCourses().isEmpty());
    }

    // ========== Import Validation Tests ==========

    void testImportValidation_EmptyName() {
        Course c;
        c.name = "";
        ScheduleConfig config;
        config.totalWeeks = 20;
        config.timeSlots = ScheduleConfig::jiangAnTimeSlots();

        auto result = ScheduleImportService::validateCourses({c}, config);
        QVERIFY(!result.valid);
        QVERIFY(!result.errors.isEmpty());
    }

    void testImportValidation_WeekOutOfRange() {
        Course c;
        c.name = "测试";
        c.endWeek = 30;
        ScheduleConfig config;
        config.totalWeeks = 20;
        config.timeSlots = ScheduleConfig::jiangAnTimeSlots();

        auto result = ScheduleImportService::validateCourses({c}, config);
        QVERIFY(!result.valid);
    }

    // ========== Course Layout Service Tests ==========

    void testMergeSameSlotCourses() {
        Course c1;
        c1.id = "m1";
        c1.name = "高等数学";
        c1.dayOfWeek = 1;
        c1.startSection = 1;
        c1.endSection = 2;
        c1.location = "一教A101";
        c1.teacher = "张老师";
        c1.weekType = WeekType::Every;

        Course c2;
        c2.id = "m2";
        c2.name = "高等数学";
        c2.dayOfWeek = 1;
        c2.startSection = 1;
        c2.endSection = 2;
        c2.location = "一教A101";
        c2.teacher = "李老师";
        c2.weekType = WeekType::Every;

        auto merged = CourseLayoutService::mergeSameSlotCourses({c1, c2});
        QCOMPARE(merged.size(), 1);
        QVERIFY(merged[0].teacher.contains("张老师"));
        QVERIFY(merged[0].teacher.contains("李老师"));
    }

    void testMergeSameSlotCoursesDoesNotAddLeadingSeparator() {
        Course first;
        first.id = "empty-teacher";
        first.name = "高等数学";
        first.dayOfWeek = 1;
        first.startSection = 1;
        first.endSection = 2;
        first.location = "一教A101";

        Course second = first;
        second.id = "named-teacher";
        second.teacher = "李老师";

        const auto merged = CourseLayoutService::mergeSameSlotCourses({first, second});
        QCOMPARE(merged.size(), 1);
        QCOMPARE(merged.first().teacher, QString("李老师"));
    }

    void testRepositoriesUseIndependentConnections() {
        ScheduleRepository first;
        ScheduleRepository second;
        first.setDatabasePath(":memory:");
        second.setDatabasePath(":memory:");

        QVERIFY(first.init());
        ScheduleConfig firstConfig = ScheduleConfig::createDefault("first");
        QVERIFY(first.addSchedule(firstConfig));

        QVERIFY(second.init());
        QVERIFY(second.allSchedules().isEmpty());

        Course course;
        course.id = "first-course";
        course.name = "高等数学";
        QVERIFY(first.addCourse(course));
        QCOMPARE(first.currentCourses().size(), 1);
        QVERIFY(second.currentCourses().isEmpty());
    }

    void testSaveUnknownScheduleConfigFails() {
        ScheduleRepository repo;
        repo.setDatabasePath(":memory:");
        QVERIFY(repo.init());

        ScheduleConfig missing = ScheduleConfig::createDefault("missing");
        QVERIFY(!repo.saveScheduleConfig(missing));
    }

    void testInvalidImportDoesNotExposePartialCourses() {
        ScheduleConfig config = ScheduleConfig::createDefault("import");

        Course valid;
        valid.name = "有效课程";
        valid.startWeek = 1;
        valid.endWeek = 10;
        valid.dayOfWeek = 1;
        valid.startSection = 1;
        valid.endSection = 2;

        Course invalid = valid;
        invalid.name.clear();

        const auto result = ScheduleImportService::validateCourses({valid, invalid}, config);
        QVERIFY(!result.valid);
        QVERIFY(result.validatedCourses.isEmpty());
    }

    void testFutureCourseFilteringDoesNotDependOnInputOrder() {
        Course future;
        future.id = "future";
        future.name = "未来课程";
        future.dayOfWeek = 1;
        future.startSection = 1;
        future.endSection = 2;
        future.startWeek = 5;
        future.endWeek = 10;

        Course active;
        active.id = "active";
        active.name = "当前课程";
        active.dayOfWeek = 1;
        active.startSection = 1;
        active.endSection = 2;
        active.startWeek = 1;
        active.endWeek = 10;

        const auto visible = CourseLayoutService::selectVisibleCoursesForWeek(
            {future, active}, 2, true);
        QCOMPARE(visible.size(), 1);
        QCOMPARE(visible.first().course.id, QString("active"));
    }

    void testFutureCourseWithSameSectionsIsHiddenEvenWithoutSharedWeeks() {
        Course active;
        active.id = "active-short";
        active.dayOfWeek = 1;
        active.startSection = 1;
        active.endSection = 2;
        active.startWeek = 1;
        active.endWeek = 3;

        Course future = active;
        future.id = "future-later";
        future.startWeek = 5;
        future.endWeek = 10;

        const auto visible = CourseLayoutService::selectVisibleCoursesForDay(
            {active, future}, 1, 2, true);
        QCOMPARE(visible.size(), 1);
        QCOMPARE(visible.first().course.id, QString("active-short"));
    }

    void testCourseEditRejectsOutOfRangeValues() {
        ScheduleRepository repo;
        repo.setDatabasePath(":memory:");
        QVERIFY(repo.init());
        QVERIFY(repo.addSchedule(ScheduleConfig::createDefault("edit")));

        CourseEditViewModel editor;
        editor.setRepository(&repo);
        editor.initForAdd(1, 1);
        editor.setName("高等数学");

        editor.setStartWeek(0);
        QVERIFY(!editor.validate());

        editor.setStartWeek(1);
        editor.setEndWeek(21);
        QVERIFY(!editor.validate());

        editor.setEndWeek(20);
        editor.setStartSection(0);
        QVERIFY(!editor.validate());
    }

    void testCourseEditBlocksConflictingNewCourse() {
        ScheduleRepository repo;
        repo.setDatabasePath(":memory:");
        QVERIFY(repo.init());
        QVERIFY(repo.addSchedule(ScheduleConfig::createDefault("conflict")));

        Course existing;
        existing.id = "existing";
        existing.name = "已有课程";
        existing.dayOfWeek = 1;
        existing.startSection = 1;
        existing.endSection = 2;
        existing.startWeek = 1;
        existing.endWeek = 20;
        QVERIFY(repo.addCourse(existing));

        CourseEditViewModel editor;
        editor.setRepository(&repo);
        editor.initForAdd(1, 1);
        editor.setName("冲突课程");
        QVERIFY(!editor.save());
        QVERIFY(editor.errorMessage().contains("已有课程"));
        QVERIFY(editor.errorMessage().contains("1-2"));
        QCOMPARE(repo.currentCourses().size(), 1);
    }

    void testCourseEditAddsAndUpdatesCourse() {
        ScheduleRepository repo;
        repo.setDatabasePath(":memory:");
        QVERIFY(repo.init());
        QVERIFY(repo.addSchedule(ScheduleConfig::createDefault("edit-success")));

        CourseEditViewModel editor;
        editor.setRepository(&repo);
        editor.initForAdd(2, 3);
        editor.setName("大学英语");
        editor.setTeacher("李老师");
        QVERIFY(editor.save());
        QCOMPARE(repo.currentCourses().size(), 1);

        const QString id = repo.currentCourses().first().id;
        editor.initForEdit("missing");
        QVERIFY(!editor.errorMessage().isEmpty());
        editor.initForEdit(id);
        QVERIFY(editor.errorMessage().isEmpty());
        editor.setLocation("一教A101");
        QVERIFY(editor.save());
        QCOMPARE(repo.currentCourses().first().location, QString("一教A101"));
    }

    void testLayoutAssignsTracksWithinOverlap() {
        Course first;
        first.id = "first";
        first.dayOfWeek = 1;
        first.startSection = 1;
        first.endSection = 2;

        Course second = first;
        second.id = "second";

        Course third = first;
        third.id = "third";
        third.startSection = 3;
        third.endSection = 4;

        QList<CourseLayoutService::LayoutCourse> courses{
            {first}, {second}, {third}
        };
        CourseLayoutService::assignCourseTracks(courses);
        QCOMPARE(courses[0].track, 0);
        QCOMPARE(courses[1].track, 1);
        QCOMPARE(courses[0].totalTracks, 2);
        QCOMPARE(courses[1].totalTracks, 2);
        QCOMPARE(courses[2].track, 0);
        QCOMPARE(courses[2].totalTracks, 1);
    }

    void testLayoutUsesConsistentWidthForChainedOverlaps() {
        Course first;
        first.id = "first";
        first.startSection = 1;
        first.endSection = 2;
        Course second = first;
        second.id = "second";
        second.startSection = 2;
        second.endSection = 3;
        Course third = first;
        third.id = "third";
        third.startSection = 3;
        third.endSection = 4;

        QList<CourseLayoutService::LayoutCourse> courses{{first}, {second}, {third}};
        CourseLayoutService::assignCourseTracks(courses);
        QCOMPARE(courses[0].track, 0);
        QCOMPARE(courses[1].track, 1);
        QCOMPARE(courses[2].track, 0);
        QCOMPARE(courses[0].totalTracks, 2);
        QCOMPARE(courses[1].totalTracks, 2);
        QCOMPARE(courses[2].totalTracks, 2);
    }

    void testCourseListModelExposesDocumentedRoles() {
        CourseListModel model;
        const QList<QByteArray> roles = model.roleNames().values();
        QVERIFY(roles.contains("id"));
        QVERIFY(roles.contains("name"));
        QVERIFY(roles.contains("teacher"));
        QVERIFY(roles.contains("track"));
        QVERIFY(roles.contains("totalTracks"));
    }

    void testScheduleViewModelManagesSchedules() {
        ScheduleRepository repo;
        repo.setDatabasePath(":memory:");

        ScheduleViewModel viewModel;
        viewModel.setRepository(&repo);
        viewModel.load();
        QVERIFY(!viewModel.hasSchedule());

        QVERIFY(viewModel.createSchedule("2026 春"));
        QVERIFY(viewModel.hasSchedule());
        QCOMPARE(viewModel.currentScheduleName(), QString("2026 春"));

        const QString id = repo.currentScheduleId();
        QVERIFY(viewModel.renameSchedule(id, "2026 春季学期"));
        QCOMPARE(viewModel.currentScheduleName(), QString("2026 春季学期"));

        QVERIFY(viewModel.deleteSchedule(id));
        QVERIFY(!viewModel.hasSchedule());
    }

    void testScheduleViewModelSavesCustomTimeSlots() {
        ScheduleRepository repo;
        repo.setDatabasePath(":memory:");

        ScheduleViewModel viewModel;
        viewModel.setRepository(&repo);
        viewModel.load();
        QVERIFY(viewModel.createSchedule("custom"));

        QVariantMap config = viewModel.currentConfig();
        QVariantList customSlots = config.value("timeSlots").toList();
        QVariantMap firstSlot = customSlots.first().toMap();
        firstSlot["startTime"] = "07:30";
        firstSlot["endTime"] = "08:15";
        customSlots[0] = firstSlot;
        config["timeSlotPreset"] = "custom";
        config["timeSlots"] = customSlots;

        QVERIFY(viewModel.saveCurrentConfig(config));
        QCOMPARE(repo.currentScheduleConfig().timeSlots.first().startTime, QTime(7, 30));
        QCOMPARE(repo.currentScheduleConfig().timeSlots.first().endTime, QTime(8, 15));
    }

    void testScheduleImportUsesInjectedRemoteApi() {
        ScheduleRepository repo;
        repo.setDatabasePath(":memory:");
        QVERIFY(repo.init());

        QJsonObject timePlace{
            {"classDay", 1},
            {"classSessions", 1},
            {"continuingSession", 2},
            {"teachingBuildingName", "一教"},
            {"classroomName", "A101"},
            {"classWeek", "11111111111111111111"},
        };
        QJsonObject detail{
            {"courseName", "高等数学"},
            {"id", QJsonObject{{"coureSequenceNumber", "01"}}},
            {"attendClassTeacher", "张老师"},
            {"timeAndPlaceList", QJsonArray{timePlace}},
        };
        const QJsonObject payload{
            {"xkxx", QJsonArray{QJsonObject{{"course", detail}}}},
        };

        ScheduleImportViewModel importer;
        importer.setRepository(&repo);
        ScheduleViewModel scheduleViewModel;
        scheduleViewModel.setRepository(&repo);
        scheduleViewModel.load();
        connect(&importer, &ScheduleImportViewModel::currentWeekSynced,
                &scheduleViewModel, [&scheduleViewModel](int week) {
                    scheduleViewModel.load();
                    scheduleViewModel.updateCurrentWeek(week);
                });
        importer.setRemoteApi(
            [](ScheduleImportViewModel::SemestersResult done) {
                done(QVariantList{QVariantMap{{"planCode", "plan-1"},
                                              {"label", "2026 春（当前）"},
                                              {"isCurrent", true}}}, {});
            },
            [payload](const QString&, ScheduleImportViewModel::ScheduleResult done) {
                done(payload, {});
            },
            [](ScheduleImportViewModel::WeekResult done) {
                done(3, {});
            });

        QVERIFY(importer.loadSemesters());
        QCOMPARE(importer.availableSemesters().size(), 1);
        QVERIFY(importer.importSchedule("plan-1", "2026 春（当前）"));
        QCOMPARE(repo.allSchedules().size(), 1);
        QCOMPARE(repo.currentScheduleConfig().semesterName, QString("2026 春"));
        QCOMPARE(repo.currentCourses().size(), 1);
        QVERIFY(importer.syncCurrentWeek());
        QCOMPARE(repo.currentScheduleConfig().semesterStartDate,
                 ScheduleImportService::calculateStartDateFromCurrentWeek(3));
        QCOMPARE(scheduleViewModel.currentWeek(), 3);
    }

    void testScheduleRepositoryPersistsAcrossRestart() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath("schedule.sqlite");

        {
            ScheduleRepository writer;
            writer.setDatabasePath(path);
            QVERIFY(writer.init());
            ScheduleConfig config = ScheduleConfig::createDefault("持久化课表");
            QVERIFY(writer.addSchedule(config));
            Course course;
            course.id = "persisted-course";
            course.name = "离线课程";
            QVERIFY(writer.addCourse(course));
        }

        ScheduleRepository reader;
        reader.setDatabasePath(path);
        QVERIFY(reader.init());
        QCOMPARE(reader.currentScheduleConfig().semesterName, QString("持久化课表"));
        QCOMPARE(reader.currentCourses().size(), 1);
        QCOMPARE(reader.currentCourses().first().name, QString("离线课程"));
    }

    void testImportSuffixStrategyCreatesUniqueNames() {
        ScheduleRepository repo;
        repo.setDatabasePath(":memory:");
        QVERIFY(repo.init());

        QJsonObject timePlace{
            {"classDay", 2}, {"classSessions", 3}, {"continuingSession", 2},
            {"classWeek", "11111111111111111111"}
        };
        QJsonObject detail{
            {"courseName", "大学英语"},
            {"timeAndPlaceList", QJsonArray{timePlace}}
        };
        const QJsonObject payload{
            {"xkxx", QJsonArray{QJsonObject{{"course", detail}}}}
        };

        ScheduleImportViewModel importer;
        importer.setRepository(&repo);
        QVERIFY(importer.importFromJson("p", "同名学期", payload));

        QVERIFY(importer.importFromJson("p", "同名学期", payload));
        QVERIFY(importer.hasConflict());
        importer.resolveConflict("addSuffix");

        QVERIFY(importer.importFromJson("p", "同名学期", payload));
        QVERIFY(importer.hasConflict());
        importer.resolveConflict("addSuffix");

        QSet<QString> names;
        for (const ScheduleConfig& config : repo.allSchedules()) names.insert(config.semesterName);
        QCOMPARE(repo.allSchedules().size(), 3);
        QCOMPARE(names.size(), 3);
    }

    void testImportUpdateExistingReplacesCoursesWithoutAddingSchedule() {
        ScheduleRepository repo;
        repo.setDatabasePath(":memory:");
        QVERIFY(repo.init());

        auto payloadForName = [](const QString& courseName) {
            QJsonObject timePlace{
                {"classDay", 3}, {"classSessions", 5}, {"continuingSession", 2},
                {"classWeek", "11111111111111111111"}
            };
            QJsonObject detail{
                {"courseName", courseName},
                {"timeAndPlaceList", QJsonArray{timePlace}}
            };
            return QJsonObject{
                {"xkxx", QJsonArray{QJsonObject{{"course", detail}}}}
            };
        };

        ScheduleImportViewModel importer;
        importer.setRepository(&repo);
        QVERIFY(importer.importFromJson("p", "更新学期", payloadForName("旧课程")));
        const QString scheduleId = repo.currentScheduleId();
        const QString oldCourseId = repo.currentCourses().first().id;

        QVERIFY(importer.importFromJson("p", "更新学期", payloadForName("新课程")));
        QVERIFY(importer.hasConflict());
        importer.resolveConflict("updateExisting");

        QCOMPARE(repo.allSchedules().size(), 1);
        QCOMPARE(repo.currentScheduleId(), scheduleId);
        QCOMPARE(repo.currentCourses().size(), 1);
        QCOMPARE(repo.currentCourses().first().name, QString("新课程"));
        QVERIFY(repo.currentCourses().first().id != oldCourseId);
    }
};

QTEST_MAIN(TestCourseModel)

#include "test_schedule.moc"
