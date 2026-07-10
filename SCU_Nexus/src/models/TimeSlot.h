#ifndef TIMESLOT_H
#define TIMESLOT_H

#include <QTime>
#include <QJsonObject>

namespace SCUNexus {

struct TimeSlot {
    QTime startTime;
    QTime endTime;

    bool isValid() const;
    QJsonObject toJson() const;
    static TimeSlot fromJson(const QJsonObject& json);

    bool operator==(const TimeSlot& other) const;
    bool operator!=(const TimeSlot& other) const;
};

} // namespace SCUNexus

#endif // TIMESLOT_H
