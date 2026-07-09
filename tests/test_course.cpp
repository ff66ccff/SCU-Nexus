#include <QtTest>
#include "../src/models/Course.h"
#include "../src/models/ScheduleConfig.h"
#include "../src/models/TimeSlot.h"
#include "../src/repositories/ScheduleRepository.h"
#include "../src/services/course/JwxtScheduleParser.h"
#include "../src/services/course/ScheduleImportService.h"
#include "../src/services/course/CourseLayoutService.h"

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

    void testJiangAnTimeSlots() {
        auto slots = ScheduleConfig::jiangAnTimeSlots();
        QCOMPARE(slots.size(), 12);

        // Check key time points
        QCOMPARE(slots[0].startTime, QTime(8, 15));
        QCOMPARE(slots[0].endTime, QTime(9, 0));
        QCOMPARE(slots[3].startTime, QTime(11, 10));
        QCOMPARE(slots[4].startTime, QTime(13, 50)); // Afternoon start
        QCOMPARE(slots[9].startTime, QTime(19, 20)); // Evening start
    }

    void testWangJiangHuaXiTimeSlots() {
        auto slots = ScheduleConfig::wangJiangHuaXiTimeSlots();
        QCOMPARE(slots.size(), 12);

        QCOMPARE(slots[0].startTime, QTime(8, 0));
        QCOMPARE(slots[3].startTime, QTime(10, 55));
        QCOMPARE(slots[4].startTime, QTime(14, 0));
        QCOMPARE(slots[9].startTime, QTime(19, 30));
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
};

QTEST_MAIN(TestCourseModel)

#include "test_course.moc"
