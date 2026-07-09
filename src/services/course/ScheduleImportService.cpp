#include "ScheduleImportService.h"
#include <QUuid>

namespace SCUNexus {

ScheduleImportService::ScheduleImportService(QObject* parent)
    : QObject(parent)
{
}

ScheduleImportService::ImportValidationResult
ScheduleImportService::validateCourses(const QList<Course>& courses, const ScheduleConfig& config)
{
    ImportValidationResult result;
    result.valid = true;

    if (config.totalWeeks < 1) {
        result.errors.append(QStringLiteral("总周数必须 >= 1"));
        result.valid = false;
    }

    if (config.timeSlots.isEmpty()) {
        result.errors.append(QStringLiteral("时间表不能为空"));
        result.valid = false;
    }

    int totalSections = config.sectionsPerDay();

    for (int i = 0; i < courses.size(); ++i) {
        const Course& course = courses[i];
        QString prefix = QStringLiteral("课程 #%1 \"%2\": ").arg(i + 1).arg(course.name);

        if (course.name.trimmed().isEmpty()) {
            result.errors.append(prefix + QStringLiteral("课程名为空"));
            result.valid = false;
            continue;
        }

        if (course.startWeek < 1) {
            result.errors.append(prefix + QStringLiteral("起始周必须 >= 1"));
            result.valid = false;
        }

        if (course.endWeek < course.startWeek) {
            result.errors.append(prefix + QStringLiteral("结束周不能小于起始周"));
            result.valid = false;
        }

        if (course.endWeek > config.totalWeeks) {
            result.errors.append(prefix + QStringLiteral("结束周(%1)超过总周数(%2)")
                .arg(course.endWeek).arg(config.totalWeeks));
            result.valid = false;
        }

        if (course.dayOfWeek < 1 || course.dayOfWeek > 7) {
            result.errors.append(prefix + QStringLiteral("星期必须在 1-7 之间"));
            result.valid = false;
        }

        if (course.startSection < 1) {
            result.errors.append(prefix + QStringLiteral("起始节次必须 >= 1"));
            result.valid = false;
        }

        if (course.endSection < course.startSection) {
            result.errors.append(prefix + QStringLiteral("结束节次不能小于起始节次"));
            result.valid = false;
        }

        if (course.endSection > totalSections) {
            result.errors.append(prefix + QStringLiteral("结束节次(%1)超过总节数(%2)")
                .arg(course.endSection).arg(totalSections));
            result.valid = false;
        }

        // Check cross-period (跨时段) rule
        int morningEnd = config.morningSections;
        int afternoonStart = morningEnd + 1;
        int afternoonEnd = morningEnd + config.afternoonSections;
        int eveningStart = afternoonEnd + 1;

        bool crossesPeriod = false;
        // Check if the course spans morning → afternoon
        if (course.startSection <= morningEnd && course.endSection >= afternoonStart) {
            crossesPeriod = true;
        }
        // Check if the course spans afternoon → evening
        if (course.startSection <= afternoonEnd && course.endSection >= eveningStart) {
            crossesPeriod = true;
        }

        if (crossesPeriod) {
            result.errors.append(prefix +
                QStringLiteral("一门课不能跨越上午、下午或晚上时段"));
            result.valid = false;
        }

        if (result.valid || result.errors.size() <= 10) {
            result.validatedCourses.append(course);
        }
    }

    return result;
}

QList<ScheduleImportService::NameConflict>
ScheduleImportService::findNameConflicts(
    const QString& newSemesterName,
    const QList<ScheduleConfig>& existingSchedules)
{
    QList<NameConflict> conflicts;
    for (const auto& schedule : existingSchedules) {
        if (schedule.semesterName == newSemesterName) {
            NameConflict conflict;
            conflict.newSemesterName = newSemesterName;
            conflict.existingScheduleId = schedule.id;
            conflict.existingScheduleName = schedule.semesterName;
            conflicts.append(conflict);
        }
    }
    return conflicts;
}

ScheduleConfig ScheduleImportService::createImportConfig(
    const QString& semesterName, QDate semesterStartDate)
{
    ScheduleConfig config;
    config.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    config.semesterName = semesterName;
    config.semesterStartDate = semesterStartDate;
    config.totalWeeks = 20;
    config.morningSections = 4;
    config.afternoonSections = 5;
    config.eveningSections = 3;
    config.courseDuration = 45;
    config.breakDuration = 10;
    config.autoSyncTime = true;
    config.timeSlots = ScheduleConfig::jiangAnTimeSlots();
    return config;
}

QDate ScheduleImportService::calculateStartDateFromCurrentWeek(
    int currentTeachingWeek, QDate today)
{
    QDate sunday = toSunday(today);
    return sunday.addDays(-(currentTeachingWeek - 1) * 7);
}

QDate ScheduleImportService::toSunday(QDate date) {
    int dayOfWeek = date.dayOfWeek(); // Qt: 1=Mon ... 7=Sun
    return date.addDays(7 - dayOfWeek);
}

} // namespace SCUNexus
