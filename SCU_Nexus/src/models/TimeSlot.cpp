#include "TimeSlot.h"

namespace SCUNexus {

bool TimeSlot::isValid() const {
    return startTime.isValid() && endTime.isValid() && startTime < endTime;
}

QJsonObject TimeSlot::toJson() const {
    QJsonObject obj;
    QJsonObject startObj;
    startObj["hour"] = startTime.hour();
    startObj["minute"] = startTime.minute();
    obj["startTime"] = startObj;

    QJsonObject endObj;
    endObj["hour"] = endTime.hour();
    endObj["minute"] = endTime.minute();
    obj["endTime"] = endObj;

    return obj;
}

TimeSlot TimeSlot::fromJson(const QJsonObject& json) {
    TimeSlot slot;
    QJsonObject startObj = json["startTime"].toObject();
    slot.startTime = QTime(startObj["hour"].toInt(), startObj["minute"].toInt());

    QJsonObject endObj = json["endTime"].toObject();
    slot.endTime = QTime(endObj["hour"].toInt(), endObj["minute"].toInt());

    return slot;
}

bool TimeSlot::operator==(const TimeSlot& other) const {
    return startTime == other.startTime && endTime == other.endTime;
}

bool TimeSlot::operator!=(const TimeSlot& other) const {
    return !(*this == other);
}

} // namespace SCUNexus
