#include "ScheduleImportService.h"
#include "CourseValidator.h"
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

    for (int i = 0; i < courses.size(); ++i) {
        const Course& course = courses[i];
        const CourseValidationResult validation = CourseValidator::validate(course, config);
        if (!validation.valid) {
            const QString prefix = QStringLiteral("课程 #%1 \"%2\": ")
                                       .arg(i + 1)
                                       .arg(course.name);
            result.errors.append(prefix + validation.message);
            result.valid = false;
        } else {
            result.validatedCourses.append(course);
        }
    }

    // Import is all-or-nothing. Never expose a partial list to callers when
    // any course or config validation failed.
    if (!result.valid) {
        result.validatedCourses.clear();
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
