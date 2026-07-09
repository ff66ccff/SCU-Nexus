#include "ScheduleConfig.h"
#include <QJsonArray>
#include <QUuid>
#include <algorithm>

namespace SCUNexus {

int ScheduleConfig::sectionsPerDay() const {
    return morningSections + afternoonSections + eveningSections;
}

int ScheduleConfig::currentWeek(QDate today) const {
    if (!semesterStartDate.isValid() || totalWeeks <= 0) {
        return 1;
    }

    if (today < semesterStartDate) {
        return 1;
    }

    qint64 days = semesterStartDate.daysTo(today);
    int week = static_cast<int>(days / 7) + 1;

    return std::clamp(week, 1, totalWeeks);
}

QJsonObject ScheduleConfig::toJson() const {
    QJsonObject obj;
    obj["id"] = id;
    obj["semesterName"] = semesterName;
    obj["semesterStartDate"] = semesterStartDate.toString(Qt::ISODate);
    obj["totalWeeks"] = totalWeeks;
    obj["morningSections"] = morningSections;
    obj["afternoonSections"] = afternoonSections;
    obj["eveningSections"] = eveningSections;
    obj["courseDuration"] = courseDuration;
    obj["breakDuration"] = breakDuration;
    obj["autoSyncTime"] = autoSyncTime;

    QJsonArray slotsArray;
    for (const auto& slot : timeSlots) {
        slotsArray.append(slot.toJson());
    }
    obj["timeSlots"] = slotsArray;

    return obj;
}

ScheduleConfig ScheduleConfig::fromJson(const QJsonObject& json) {
    ScheduleConfig config;
    config.id = json["id"].toString();
    config.semesterName = json["semesterName"].toString();
    config.semesterStartDate = QDate::fromString(json["semesterStartDate"].toString(), Qt::ISODate);
    config.totalWeeks = json["totalWeeks"].toInt(20);
    config.morningSections = json["morningSections"].toInt(4);
    config.afternoonSections = json["afternoonSections"].toInt(5);
    config.eveningSections = json["eveningSections"].toInt(3);
    config.courseDuration = json["courseDuration"].toInt(45);
    config.breakDuration = json["breakDuration"].toInt(10);
    config.autoSyncTime = json["autoSyncTime"].toBool(true);

    QJsonArray slotsArray = json["timeSlots"].toArray();
    for (const auto& val : slotsArray) {
        config.timeSlots.append(TimeSlot::fromJson(val.toObject()));
    }

    return config;
}

QVector<TimeSlot> ScheduleConfig::jiangAnTimeSlots() {
    return {
        {QTime(8, 15),  QTime(9, 0)},
        {QTime(9, 10),  QTime(9, 55)},
        {QTime(10, 15), QTime(11, 0)},
        {QTime(11, 10), QTime(11, 55)},
        {QTime(13, 50), QTime(14, 35)},
        {QTime(14, 45), QTime(15, 30)},
        {QTime(15, 40), QTime(16, 25)},
        {QTime(16, 45), QTime(17, 30)},
        {QTime(17, 40), QTime(18, 25)},
        {QTime(19, 20), QTime(20, 5)},
        {QTime(20, 15), QTime(21, 0)},
        {QTime(21, 10), QTime(21, 55)},
    };
}

QVector<TimeSlot> ScheduleConfig::wangJiangHuaXiTimeSlots() {
    return {
        {QTime(8, 0),  QTime(8, 45)},
        {QTime(8, 55),  QTime(9, 40)},
        {QTime(10, 0), QTime(10, 45)},
        {QTime(10, 55), QTime(11, 40)},
        {QTime(14, 0), QTime(14, 45)},
        {QTime(14, 55), QTime(15, 40)},
        {QTime(15, 50), QTime(16, 35)},
        {QTime(16, 55), QTime(17, 40)},
        {QTime(17, 50), QTime(18, 35)},
        {QTime(19, 30), QTime(20, 15)},
        {QTime(20, 25), QTime(21, 10)},
        {QTime(21, 20), QTime(22, 5)},
    };
}

std::optional<QVector<TimeSlot>> ScheduleConfig::timeSlotsForCampusName(const QString& campusName) {
    if (campusName.contains(QStringLiteral("江安"))) {
        return jiangAnTimeSlots();
    }
    if (campusName.contains(QStringLiteral("望江")) || campusName.contains(QStringLiteral("华西"))) {
        return wangJiangHuaXiTimeSlots();
    }
    return std::nullopt;
}

ScheduleConfig ScheduleConfig::createDefault(const QString& name) {
    ScheduleConfig config;
    config.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    config.semesterName = name.isEmpty() ? QStringLiteral("新课程表") : name;
    config.semesterStartDate = QDate::currentDate();
    config.totalWeeks = 20;
    config.morningSections = 4;
    config.afternoonSections = 5;
    config.eveningSections = 3;
    config.courseDuration = 45;
    config.breakDuration = 10;
    config.autoSyncTime = true;
    config.timeSlots = jiangAnTimeSlots();
    return config;
}

bool ScheduleConfig::operator==(const ScheduleConfig& other) const {
    return id == other.id;
}

bool ScheduleConfig::operator!=(const ScheduleConfig& other) const {
    return !(*this == other);
}

} // namespace SCUNexus
