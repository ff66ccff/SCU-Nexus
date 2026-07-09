#include "JwxtScheduleParser.h"
#include <QUuid>
#include <QDebug>

namespace SCUNexus {

const QList<quint32> JwxtScheduleParser::COLOR_POOL = {
    0xFFEF5350, 0xFFEC407A, 0xFFAB47BC, 0xFF7E57C2,
    0xFF5C6BC0, 0xFF42A5F5, 0xFF26C6DA, 0xFF26A69A,
    0xFF66BB6A, 0xFF9CCC65, 0xFFFFA726, 0xFF8D6E63,
};

QString JwxtScheduleParser::cleanSemesterLabel(const QString& label) {
    QString cleaned = label;
    cleaned.replace(QStringLiteral("（当前）"), QString());
    cleaned.replace(QStringLiteral("(当前)"), QString());
    return cleaned.trimmed();
}

JwxtScheduleParser::ParseResult JwxtScheduleParser::parse(const QJsonObject& rawJson) {
    ParseResult result;
    result.success = false;

    QJsonArray xkxx = rawJson["xkxx"].toArray();
    if (xkxx.isEmpty()) {
        result.errorMessage = QStringLiteral("导入数据格式异常：xkxx 数组为空或不存在");
        return result;
    }

    int colorIndex = 0;
    int generatedCount = 0;

    // Iterate xkxx array
    for (const auto& xkxxItem : xkxx) {
        QJsonObject xkxxObj = xkxxItem.toObject();

        // Iterate each key/value in the object (course map)
        for (auto it = xkxxObj.begin(); it != xkxxObj.end(); ++it) {
            QJsonObject courseDetail = it.value().toObject();
            if (courseDetail.isEmpty()) continue;

            // Extract course name and sequence number
            QString courseName = courseDetail["courseName"].toString();
            QString coureSequenceNumber;
            QJsonObject idObj = courseDetail["id"].toObject();
            if (!idObj.isEmpty()) {
                coureSequenceNumber = idObj["coureSequenceNumber"].toString();
            }

            QString displayName;
            if (!coureSequenceNumber.isEmpty()) {
                displayName = courseName + QStringLiteral(" (") + coureSequenceNumber + QStringLiteral(")");
            } else {
                displayName = courseName;
            }

            QString teacher = courseDetail["attendClassTeacher"].toString();

            // Parse timeAndPlaceList
            QJsonArray timePlaceList = courseDetail["timeAndPlaceList"].toArray();
            if (timePlaceList.isEmpty()) {
                result.warnings.append(
                    QStringLiteral("课程 \"%1\" 没有时间地点信息，已跳过").arg(displayName));
                continue;
            }

            for (const auto& tpVal : timePlaceList) {
                QJsonObject tpObj = tpVal.toObject();
                auto courseOpt = parseTimeAndPlace(tpObj, courseName, displayName, teacher);

                if (!courseOpt.has_value()) {
                    result.warnings.append(
                        QStringLiteral("课程 \"%1\" 的某个时间地点解析失败，已跳过").arg(displayName));
                    continue;
                }

                Course course = courseOpt.value();
                // Assign color from pool
                course.colorValue = COLOR_POOL[colorIndex % COLOR_POOL.size()];
                colorIndex++;

                result.courses.append(course);
                generatedCount++;
            }
        }
    }

    if (result.courses.isEmpty()) {
        result.errorMessage = QStringLiteral("导入数据格式异常：未能解析出任何课程");
        return result;
    }

    result.success = true;
    return result;
}

std::optional<Course> JwxtScheduleParser::parseTimeAndPlace(
    const QJsonObject& tpObj,
    const QString& courseName,
    const QString& displayName,
    const QString& teacher)
{
    int classDay = tpObj["classDay"].toInt(0);
    int classSessions = tpObj["classSessions"].toInt(0);
    int continuingSession = tpObj["continuingSession"].toInt(1);
    QString teachingBuilding = tpObj["teachingBuildingName"].toString();
    QString classroomName = tpObj["classroomName"].toString();
    QString classWeek = tpObj["classWeek"].toString();

    // Basic validation
    if (classDay < 1 || classDay > 7 || classSessions < 1) {
        return std::nullopt;
    }

    auto weekInfo = parseClassWeek(classWeek);
    if (!weekInfo.has_value()) {
        return std::nullopt;
    }

    Course course;
    course.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    course.name = displayName;
    course.teacher = teacher;
    course.location = teachingBuilding + classroomName;
    course.dayOfWeek = classDay;
    course.startSection = classSessions;
    course.endSection = classSessions + continuingSession - 1;
    course.startWeek = weekInfo->startWeek;
    course.endWeek = weekInfo->endWeek;
    course.weekType = weekInfo->weekType;

    return course;
}

std::optional<JwxtScheduleParser::WeekInfo> JwxtScheduleParser::parseClassWeek(const QString& classWeek) {
    if (classWeek.isEmpty()) {
        return std::nullopt;
    }

    QList<int> activeWeeks;
    for (int i = 0; i < classWeek.length(); ++i) {
        if (classWeek[i] == '1') {
            activeWeeks.append(i + 1);
        }
    }

    if (activeWeeks.isEmpty()) {
        return std::nullopt;
    }

    WeekInfo info;
    info.activeWeeks = activeWeeks;
    info.startWeek = activeWeeks.first();
    info.endWeek = activeWeeks.last();

    // Determine week type
    bool allOdd = true;
    bool allEven = true;
    for (int week : activeWeeks) {
        if (week % 2 == 0) allOdd = false;
        if (week % 2 != 0) allEven = false;
    }

    if (allOdd) {
        info.weekType = WeekType::Odd;
    } else if (allEven) {
        info.weekType = WeekType::Even;
    } else {
        info.weekType = WeekType::Every;
    }

    return info;
}

} // namespace SCUNexus
