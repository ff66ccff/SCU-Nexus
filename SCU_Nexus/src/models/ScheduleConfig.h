#ifndef SCHEDULECONFIG_H
#define SCHEDULECONFIG_H

#include <QString>
#include <QDate>
#include <QVector>
#include <QJsonObject>
#include <optional>
#include "TimeSlot.h"

namespace SCUNexus {

class ScheduleConfig {
public:
    QString id;
    QString semesterName;
    QDate semesterStartDate;
    int totalWeeks = 20;
    int morningSections = 4;
    int afternoonSections = 5;
    int eveningSections = 3;
    int courseDuration = 45;   // minutes
    int breakDuration = 10;    // minutes
    bool autoSyncTime = true;
    QVector<TimeSlot> timeSlots;

    // Derived properties
    int sectionsPerDay() const;
    int currentWeek(QDate today = QDate::currentDate()) const;

    // JSON serialization (compatible with Flutter)
    QJsonObject toJson() const;
    static ScheduleConfig fromJson(const QJsonObject& json);

    // Default time slot presets
    static QVector<TimeSlot> jiangAnTimeSlots();
    static QVector<TimeSlot> wangJiangHuaXiTimeSlots();
    static std::optional<QVector<TimeSlot>> timeSlotsForCampusName(const QString& campusName);

    // Create a default config with Jiang'an time slots
    static ScheduleConfig createDefault(const QString& name = QString());

    bool operator==(const ScheduleConfig& other) const;
    bool operator!=(const ScheduleConfig& other) const;
};

} // namespace SCUNexus

#endif // SCHEDULECONFIG_H
