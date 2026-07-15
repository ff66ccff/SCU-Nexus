#include "src/models/ClassroomModels.h"

#include <QtTest>

class ClassroomQueryTests final : public QObject
{
    Q_OBJECT

private slots:
    void expandsContinuousPeriodsAndMapsStatuses();
    void computesCurrentPeriodForCampusTimetable();
};

void ClassroomQueryTests::expandsContinuousPeriodsAndMapsStatuses()
{
    ClassroomQueryResultDto result;
    result.timeSlots = {
        ClassroomTimeSlotDto{QStringLiteral("01"),
                             QStringLiteral("101"),
                             QStringLiteral("A101"),
                             3,
                             2,
                             2,
                             QStringLiteral("busy"),
                             QStringLiteral("06")},
        ClassroomTimeSlotDto{QStringLiteral("01"),
                             QStringLiteral("101"),
                             QStringLiteral("A101"),
                             3,
                             5,
                             1,
                             QStringLiteral("busy"),
                             QStringLiteral("07")},
        ClassroomTimeSlotDto{QStringLiteral("01"),
                             QStringLiteral("101"),
                             QStringLiteral("A101"),
                             3,
                             6,
                             1,
                             QStringLiteral("busy"),
                             QStringLiteral("14")},
        ClassroomTimeSlotDto{QStringLiteral("01"),
                             QStringLiteral("101"),
                             QStringLiteral("A101"),
                             3,
                             7,
                             1,
                             QStringLiteral("busy"),
                             QStringLiteral("room")},
    };

    const QVector<ClassroomPeriodStatus> statuses =
        classroomPeriodStatuses(result, QStringLiteral("A101"));

    QCOMPARE(statuses.size(), 12);
    QCOMPARE(statuses.at(0), ClassroomPeriodStatus::Free);
    QCOMPARE(statuses.at(1), ClassroomPeriodStatus::InClass);
    QCOMPARE(statuses.at(2), ClassroomPeriodStatus::InClass);
    QCOMPARE(statuses.at(4), ClassroomPeriodStatus::Exam);
    QCOMPARE(statuses.at(5), ClassroomPeriodStatus::Experiment);
    QCOMPARE(statuses.at(6), ClassroomPeriodStatus::Borrowed);
    QCOMPARE(classroomStatusKey(statuses.at(4)), QStringLiteral("exam"));
    QCOMPARE(classroomStatusText(statuses.at(6)), QStringLiteral("借用"));
}

void ClassroomQueryTests::computesCurrentPeriodForCampusTimetable()
{
    QCOMPARE(currentClassroomPeriod(QStringLiteral("江安"), QTime(8, 0)), 1);
    QCOMPARE(currentClassroomPeriod(QStringLiteral("江安"), QTime(9, 5)), 2);
    QCOMPARE(currentClassroomPeriod(QStringLiteral("望江"), QTime(13, 50)), 5);
    QCOMPARE(currentClassroomPeriod(QStringLiteral("未知校区"), QTime(8, 0)), 0);
    QCOMPARE(currentClassroomPeriod(QStringLiteral("江安"), QTime(23, 30)), 0);
}

QTEST_APPLESS_MAIN(ClassroomQueryTests)

#include "test_classroom_query.moc"
