#include "Course.h"
#include <QUuid>
#include <algorithm>

namespace SCUNexus {

bool Course::isInWeekRange(int week) const {
    return week >= startWeek && week <= endWeek;
}

bool Course::isActiveInWeek(int week) const {
    if (!isInWeekRange(week)) {
        return false;
    }
    if (weekType == WeekType::Odd && week % 2 == 0) {
        return false;
    }
    if (weekType == WeekType::Even && week % 2 != 0) {
        return false;
    }
    return true;
}

bool Course::hasSharedWeekWith(const Course& other) const {
    // Quick rejection: no week range overlap at all
    int overlapStart = std::max(startWeek, other.startWeek);
    int overlapEnd = std::min(endWeek, other.endWeek);
    if (overlapStart > overlapEnd) {
        return false;
    }

    // Check if there's at least one common week within the overlap
    // considering week type constraints
    for (int week = overlapStart; week <= overlapEnd; ++week) {
        if (isActiveInWeek(week) && other.isActiveInWeek(week)) {
            return true;
        }
    }
    return false;
}

bool Course::conflictsWith(const Course& other, const QString& excludeId) const {
    // Self-check: exclude by id
    if (!excludeId.isEmpty() && id == excludeId) {
        return false;
    }

    // Different day of week -> no conflict
    if (dayOfWeek != other.dayOfWeek) {
        return false;
    }

    // Section ranges don't overlap
    if (endSection < other.startSection || other.endSection < startSection) {
        return false;
    }

    // Week ranges don't overlap
    if (endWeek < other.startWeek || other.endWeek < startWeek) {
        return false;
    }

    // Mutually exclusive week types (e.g. odd-only vs even-only -> no shared weeks)
    if ((weekType == WeekType::Odd && other.weekType == WeekType::Even) ||
        (weekType == WeekType::Even && other.weekType == WeekType::Odd)) {
        return false;
    }

    // Shared weeks must actually be active
    if (!hasSharedWeekWith(other)) {
        return false;
    }

    return true;
}

QJsonObject Course::toJson() const {
    QJsonObject obj;
    obj["id"] = id;
    obj["name"] = name;
    obj["teacher"] = teacher;
    obj["location"] = location;
    obj["startWeek"] = startWeek;
    obj["endWeek"] = endWeek;
    obj["dayOfWeek"] = dayOfWeek;
    obj["startSection"] = startSection;
    obj["endSection"] = endSection;
    obj["colorValue"] = static_cast<int>(colorValue);
    obj["weekType"] = static_cast<int>(weekType);
    return obj;
}

Course Course::fromJson(const QJsonObject& json) {
    Course course;
    course.id = json["id"].toString();
    course.name = json["name"].toString();
    course.teacher = json["teacher"].toString();
    course.location = json["location"].toString();
    course.startWeek = json["startWeek"].toInt(1);
    course.endWeek = json["endWeek"].toInt(20);
    course.dayOfWeek = json["dayOfWeek"].toInt(1);
    course.startSection = json["startSection"].toInt(1);
    course.endSection = json["endSection"].toInt(2);
    course.colorValue = static_cast<quint32>(json["colorValue"].toInt(0xFF42A5F5));
    course.weekType = intToWeekType(json["weekType"].toInt(0));
    return course;
}

Course Course::copyWith(std::optional<QString> newId,
                         std::optional<QString> newName,
                         std::optional<QString> newTeacher,
                         std::optional<QString> newLocation,
                         std::optional<int> newStartWeek,
                         std::optional<int> newEndWeek,
                         std::optional<int> newDayOfWeek,
                         std::optional<int> newStartSection,
                         std::optional<int> newEndSection,
                         std::optional<quint32> newColorValue,
    std::optional<WeekType> newWeekType) const {
    Course copy = *this;
    if (newId.has_value()) copy.id = newId.value();
    if (newName.has_value()) copy.name = newName.value();
    if (newTeacher.has_value()) copy.teacher = newTeacher.value();
    if (newLocation.has_value()) copy.location = newLocation.value();
    if (newStartWeek.has_value()) copy.startWeek = newStartWeek.value();
    if (newEndWeek.has_value()) copy.endWeek = newEndWeek.value();
    if (newDayOfWeek.has_value()) copy.dayOfWeek = newDayOfWeek.value();
    if (newStartSection.has_value()) copy.startSection = newStartSection.value();
    if (newEndSection.has_value()) copy.endSection = newEndSection.value();
    if (newColorValue.has_value()) copy.colorValue = newColorValue.value();
    if (newWeekType.has_value()) copy.weekType = newWeekType.value();
    return copy;
}

bool Course::operator==(const Course& other) const {
    return id == other.id;
}

bool Course::operator!=(const Course& other) const {
    return !(*this == other);
}

} // namespace SCUNexus
