#include "CourseValidator.h"

namespace SCUNexus {

CourseValidationResult CourseValidator::validate(const Course &course,
                                                 const ScheduleConfig &config,
                                                 const QList<Course> &existing,
                                                 const QString &excludeId)
{
    if (course.name.trimmed().isEmpty()) {
        return {false, QStringLiteral("课程名不能为空"), {}};
    }

    if (course.dayOfWeek < 1 || course.dayOfWeek > 7) {
        return {false, QStringLiteral("星期必须在 1 到 7 之间"), {}};
    }

    if (course.startWeek < 1 || course.endWeek < 1
        || course.startWeek > config.totalWeeks || course.endWeek > config.totalWeeks) {
        return {false,
                QStringLiteral("周次必须在 1 到 %1 之间").arg(config.totalWeeks),
                {}};
    }

    if (course.startWeek > course.endWeek) {
        return {false, QStringLiteral("起始周不能大于结束周"), {}};
    }

    if (course.startSection < 1 || course.endSection < 1) {
        return {false, QStringLiteral("节次必须从 1 开始"), {}};
    }

    const int totalSections = config.sectionsPerDay();
    if (course.startSection > totalSections || course.endSection > totalSections) {
        return {false,
                QStringLiteral("节次不能超过当前配置（%1）").arg(totalSections),
                {}};
    }

    if (course.startSection > course.endSection) {
        return {false, QStringLiteral("起始节不能大于结束节"), {}};
    }

    const int morningEnd = config.morningSections;
    const int afternoonEnd = morningEnd + config.afternoonSections;
    const bool crossesMorningBoundary = course.startSection <= morningEnd
        && course.endSection > morningEnd;
    const bool crossesAfternoonBoundary = course.startSection <= afternoonEnd
        && course.endSection > afternoonEnd;
    if (crossesMorningBoundary || crossesAfternoonBoundary) {
        return {false,
                QStringLiteral("一门课不能跨越上午、下午或晚上时段。"),
                {}};
    }

    for (const Course &existingCourse : existing) {
        if (!excludeId.isEmpty() && existingCourse.id == excludeId) {
            continue;
        }
        if (course.conflictsWith(existingCourse)) {
            return {
                false,
                QStringLiteral("课程时间冲突：与“%1”（周%2，第 %3-%4 节）冲突。")
                    .arg(existingCourse.name)
                    .arg(existingCourse.dayOfWeek)
                    .arg(existingCourse.startSection)
                    .arg(existingCourse.endSection),
                existingCourse.id,
            };
        }
    }

    return {true, {}, {}};
}

} // namespace SCUNexus
