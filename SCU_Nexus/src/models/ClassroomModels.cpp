#include "src/models/ClassroomModels.h"

#include "src/models/ScheduleConfig.h"

#include <algorithm>

ClassroomPeriodStatus classroomStatusForModule(const QString& moduleId)
{
    if (moduleId == QStringLiteral("06")) {
        return ClassroomPeriodStatus::InClass;
    }
    if (moduleId == QStringLiteral("07")) {
        return ClassroomPeriodStatus::Exam;
    }
    if (moduleId == QStringLiteral("14")) {
        return ClassroomPeriodStatus::Experiment;
    }
    if (moduleId == QStringLiteral("room")) {
        return ClassroomPeriodStatus::Borrowed;
    }
    return ClassroomPeriodStatus::Free;
}

QString classroomStatusKey(ClassroomPeriodStatus status)
{
    switch (status) {
    case ClassroomPeriodStatus::Free:
        return QStringLiteral("free");
    case ClassroomPeriodStatus::InClass:
        return QStringLiteral("inClass");
    case ClassroomPeriodStatus::Exam:
        return QStringLiteral("exam");
    case ClassroomPeriodStatus::Experiment:
        return QStringLiteral("experiment");
    case ClassroomPeriodStatus::Borrowed:
        return QStringLiteral("borrowed");
    }
    return QStringLiteral("free");
}

QString classroomStatusText(ClassroomPeriodStatus status)
{
    switch (status) {
    case ClassroomPeriodStatus::Free:
        return QStringLiteral("空闲");
    case ClassroomPeriodStatus::InClass:
        return QStringLiteral("上课");
    case ClassroomPeriodStatus::Exam:
        return QStringLiteral("考试");
    case ClassroomPeriodStatus::Experiment:
        return QStringLiteral("实验");
    case ClassroomPeriodStatus::Borrowed:
        return QStringLiteral("借用");
    }
    return QStringLiteral("空闲");
}

QVector<ClassroomPeriodStatus> classroomPeriodStatuses(
    const ClassroomQueryResultDto& result,
    const QString& classroomNumber)
{
    QVector<ClassroomPeriodStatus> statuses(12, ClassroomPeriodStatus::Free);
    for (const ClassroomTimeSlotDto& slot : result.timeSlots) {
        if (slot.classroomNumber != classroomNumber || slot.sessionStart < 1) {
            continue;
        }
        const int first = std::clamp(slot.sessionStart, 1, 12);
        const int last = std::clamp(slot.sessionStart + qMax(1, slot.continuingSession) - 1,
                                    1,
                                    12);
        const ClassroomPeriodStatus status = classroomStatusForModule(slot.occupancyModuleId);
        for (int period = first; period <= last; ++period) {
            statuses[period - 1] = status;
        }
    }
    return statuses;
}

int currentClassroomPeriod(const QString& campusName, const QTime& now)
{
    const auto campusSlots = SCUNexus::ScheduleConfig::timeSlotsForCampusName(campusName);
    if (!campusSlots || campusSlots->isEmpty() || !now.isValid()) {
        return 0;
    }

    const int currentMinutes = now.hour() * 60 + now.minute();
    constexpr int PreClassLeadMinutes = 15;
    for (qsizetype index = 0; index < campusSlots->size(); ++index) {
        const SCUNexus::TimeSlot& slot = campusSlots->at(index);
        const int startMinutes = slot.startTime.hour() * 60 + slot.startTime.minute();
        const int endMinutes = slot.endTime.hour() * 60 + slot.endTime.minute();
        if (currentMinutes >= startMinutes && currentMinutes < endMinutes) {
            return static_cast<int>(index) + 1;
        }
        if (index == 0 && currentMinutes >= startMinutes - PreClassLeadMinutes
            && currentMinutes < startMinutes) {
            return 1;
        }
        if (index + 1 < campusSlots->size()) {
            const SCUNexus::TimeSlot& next = campusSlots->at(index + 1);
            const int nextStartMinutes = next.startTime.hour() * 60 + next.startTime.minute();
            if (currentMinutes >= endMinutes && currentMinutes < nextStartMinutes) {
                return static_cast<int>(index) + 2;
            }
        }
    }
    return 0;
}
