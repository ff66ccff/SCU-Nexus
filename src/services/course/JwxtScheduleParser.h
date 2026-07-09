#ifndef JWXTSCHEDULEPARSER_H
#define JWXTSCHEDULEPARSER_H

#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <QString>
#include "../../models/Course.h"

namespace SCUNexus {

// Parses raw JSON from the teaching affairs system (教务系统) into Course objects.
class JwxtScheduleParser {
public:
    // Color pool for course assignment (ARGB), matching Flutter palette
    static const QList<quint32> COLOR_POOL;

    struct ParseResult {
        QList<Course> courses;
        QStringList warnings;  // Non-fatal issues encountered
        bool success = false;
        QString errorMessage;
    };

    // Parse the full xkxx JSON array into a list of courses
    static ParseResult parse(const QJsonObject& rawJson);

    // Clean semester label: remove "（当前）" / "(当前)" and trim
    static QString cleanSemesterLabel(const QString& label);

private:
    // Parse a single timeAndPlace entry into a Course
    static std::optional<Course> parseTimeAndPlace(
        const QJsonObject& timePlaceObj,
        const QString& courseName,
        const QString& displayName,
        const QString& teacher);

    // Parse the classWeek bit string into startWeek, endWeek, weekType
    struct WeekInfo {
        int startWeek = 0;
        int endWeek = 0;
        WeekType weekType = WeekType::Every;
        QList<int> activeWeeks;
    };
    static std::optional<WeekInfo> parseClassWeek(const QString& classWeek);
};

} // namespace SCUNexus

#endif // JWXTSCHEDULEPARSER_H
