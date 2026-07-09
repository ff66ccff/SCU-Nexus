#ifndef COURSE_H
#define COURSE_H

#include <QString>
#include <QJsonObject>
#include "WeekType.h"

namespace SCUNexus {

class Course {
public:
    QString id;
    QString name;
    QString teacher;
    QString location;
    int startWeek = 1;
    int endWeek = 20;
    int dayOfWeek = 1;      // 1=Mon ... 7=Sun
    int startSection = 1;
    int endSection = 2;
    quint32 colorValue = 0xFF42A5F5; // ARGB
    WeekType weekType = WeekType::Every;

    // Core logic
    bool isInWeekRange(int week) const;
    bool isActiveInWeek(int week) const;
    bool conflictsWith(const Course& other, const QString& excludeId = {}) const;

    // Helper: check if two courses have any shared weeks (for conflict detection)
    bool hasSharedWeekWith(const Course& other) const;

    // JSON serialization (compatible with Flutter)
    QJsonObject toJson() const;
    static Course fromJson(const QJsonObject& json);

    // Create a copy with modified fields
    Course copyWith(const QString& newId = {},
                    const QString& newName = {},
                    const QString& newTeacher = {},
                    const QString& newLocation = {},
                    std::optional<int> newStartWeek = {},
                    std::optional<int> newEndWeek = {},
                    std::optional<int> newDayOfWeek = {},
                    std::optional<int> newStartSection = {},
                    std::optional<int> newEndSection = {},
                    std::optional<quint32> newColorValue = {},
                    std::optional<WeekType> newWeekType = {}) const;

    bool operator==(const Course& other) const;
    bool operator!=(const Course& other) const;
};

} // namespace SCUNexus

#endif // COURSE_H
