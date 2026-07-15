#include "ZhjwParsers.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>

namespace {

// 教务页面不是严格 XML；这里只做字段级轻量清洗，不承担通用 HTML 解析。
QString stripTags(QString text)
{
    text.remove(QRegularExpression("<[^>]+>"));
    text.replace("&nbsp;", " ");
    text.replace("&amp;", "&");
    text.replace("&lt;", "<");
    text.replace("&gt;", ">");
    return text.simplified();
}

QString firstCapture(const QString& text, const QString& pattern)
{
    const QRegularExpression regex(pattern, QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch match = regex.match(text);
    return match.hasMatch() ? stripTags(match.captured(1)) : QString();
}

// 从文本中提取指定标签后的值。
QString labeledValue(const QString& text, const QString& label)
{
    return firstCapture(text, label + R"(\s*[:：]\s*(?:&nbsp;|\s)*([^<]+))");
}

// callback 中间段由服务端动态生成，只约束固定前缀和业务后缀。
QString extractCallback(const QString& html, const QString& suffix)
{
    const QRegularExpression regex(
        QStringLiteral("var\\s+url\\s*=\\s*\"(/student/integratedQuery/scoreQuery/[^/]+/%1/callback)\"")
            .arg(suffix));
    const QRegularExpressionMatch match = regex.match(html);
    return match.hasMatch() ? match.captured(1) : QString();
}

bool failParse(QString* errorMessage, const QString& message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
    return false;
}

QString hiddenJsonValue(const QString& html, const QString& id)
{
    const QRegularExpression regex(
        QStringLiteral("<input[^>]*id\\s*=\\s*[\"']%1[\"'][^>]*value\\s*=\\s*'([^']*)'[^>]*>")
            .arg(QRegularExpression::escape(id)),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = regex.match(html);
    return match.hasMatch() ? match.captured(1) : QString();
}

QString jsonString(const QJsonValue& value)
{
    return value.isString() ? value.toString() : value.toVariant().toString();
}

int jsonInt(const QJsonValue& value, int fallback = 0)
{
    if (value.isDouble()) {
        return value.toInt(fallback);
    }
    bool ok = false;
    const int parsed = value.toString().toInt(&ok);
    return ok ? parsed : fallback;
}

}

namespace ZhjwParsers {

// 教务会话过期没有统一响应格式：重定向、空白 200、英文 login 页和中文统一认证页都可能出现。
bool isSessionExpired(const QString& body, int statusCode)
{
    const QString trimmed = body.trimmed();
    if (statusCode == 302 || trimmed.isEmpty()) {
        return true;
    }
    if (trimmed.startsWith('<') && trimmed.contains("login", Qt::CaseInsensitive)) {
        return true;
    }
    return trimmed.contains("统一身份认证") || trimmed.contains("用户登录");
}

// 未匹配时返回 0，由服务层统一映射为 ParseFailed，而不是猜测教学周。
int parseCurrentWeek(const QString& html)
{
    const QRegularExpression regex(QStringLiteral("第(\\d+)周"));
    const QRegularExpressionMatch match = regex.match(html);
    return match.hasMatch() ? match.captured(1).toInt() : 0;
}

// 保持服务端 option 顺序和 label 原文；当前学期标记由课表导入层处理。
QList<SemesterDto> parseSemesters(const QString& html)
{
    QList<SemesterDto> semesters;
    const QRegularExpression regex(
        QStringLiteral("<option[^>]+value\\s*=\\s*\"([^\"]+)\"[^>]*>(.*?)</option>"),
        QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator matches = regex.globalMatch(html);
    while (matches.hasNext()) {
        const QRegularExpressionMatch match = matches.next();
        semesters.append({match.captured(1).trimmed(), stripTags(match.captured(2))});
    }
    return semesters;
}

// 解析考表条目，并区分“确实无考试”和“页面改版/返回了错误页面”。
ExamPlanParseResult parseExamPlanResult(const QString& html)
{
    ExamPlanParseResult result;
    result.explicitlyEmpty = html.contains(QStringLiteral("暂无考试安排"))
        || html.contains(QStringLiteral("暂无数据"))
        || html.contains(QStringLiteral("没有查询到"));

    // 以相邻 widget-box 为卡片边界，避免一个全局正则跨卡片串联字段。
    const QRegularExpression blockRegex(
        R"((<div[^>]+class\s*=\s*"[^"]*widget-box\s+widget-color-[^"]*"[^>]*>.*?)(?=<div[^>]+class\s*=\s*"[^"]*widget-box\s+widget-color-|$))",
        QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator blocks = blockRegex.globalMatch(html);
    while (blocks.hasNext()) {
        result.recognized = true;
        const QString block = blocks.next().captured(1);
        ExamPlanItemDto item;
        item.courseName = firstCapture(
            block,
            QStringLiteral("<h5[^>]*class\\s*=\\s*\"[^\"]*widget-title\\s+smaller[^\"]*\"[^>]*>(.*?)</h5>"));
        item.courseName.remove(QStringLiteral("（已结束）"));
        item.courseName = item.courseName.trimmed();
        item.week = firstCapture(block, QStringLiteral("第(\\d+)周"));
        item.date = firstCapture(block, QStringLiteral("(\\d{4}-\\d{2}-\\d{2})"));
        item.weekday = firstCapture(block, QStringLiteral("(星期[一二三四五六日])"));
        item.timeRange = firstCapture(block, QStringLiteral("(\\d{2}:\\d{2}-\\d{2}:\\d{2})"));
        item.location = labeledValue(block, QStringLiteral("地点"));
        item.seatNumber = labeledValue(block, QStringLiteral("座位号"));
        item.ticketNumber = labeledValue(block, QStringLiteral("准考证号"));
        item.tip = labeledValue(block, QStringLiteral("考试提示信息"));
        if (item.tip.isEmpty()) {
            item.tip = QStringLiteral("无");
        }
        if (!item.courseName.isEmpty()) {
            result.items.append(item);
        }
    }
    return result;
}

// 保留旧调用接口；需要区分空态与解析失败的新代码应使用 parseExamPlanResult。
QList<ExamPlanItemDto> parseExamPlan(const QString& html)
{
    return parseExamPlanResult(html).items;
}

bool parseClassroomIndex(const QString& html,
                         ClassroomIndexDto* result,
                         QString* errorMessage)
{
    if (!result) {
        return failParse(errorMessage, QStringLiteral("教室索引输出参数无效"));
    }

    const QString campusJson = hiddenJsonValue(html, QStringLiteral("xqList"));
    const QString buildingJson = hiddenJsonValue(html, QStringLiteral("jxlList"));
    if (campusJson.isEmpty() || buildingJson.isEmpty()) {
        return failParse(errorMessage, QStringLiteral("教室查询页面缺少校区或教学楼数据"));
    }

    QJsonParseError campusError;
    QJsonParseError buildingError;
    const QJsonDocument campusDocument = QJsonDocument::fromJson(campusJson.toUtf8(), &campusError);
    const QJsonDocument buildingDocument = QJsonDocument::fromJson(buildingJson.toUtf8(), &buildingError);
    if (campusError.error != QJsonParseError::NoError || !campusDocument.isArray()
        || buildingError.error != QJsonParseError::NoError || !buildingDocument.isArray()) {
        return failParse(errorMessage, QStringLiteral("校区或教学楼数据格式无效"));
    }

    ClassroomIndexDto parsed;
    for (const QJsonValue& value : campusDocument.array()) {
        if (!value.isObject()) {
            return failParse(errorMessage, QStringLiteral("校区条目格式无效"));
        }
        const QJsonObject object = value.toObject();
        ClassroomCampusDto campus{
            object.value(QStringLiteral("campusName")).toString().trimmed(),
            jsonString(object.value(QStringLiteral("campusNumber"))).trimmed()
        };
        if (campus.campusName.isEmpty() || campus.campusNumber.isEmpty()) {
            return failParse(errorMessage, QStringLiteral("校区条目缺少名称或编号"));
        }
        parsed.campuses.append(campus);
    }

    for (const QJsonValue& value : buildingDocument.array()) {
        if (!value.isObject()) {
            return failParse(errorMessage, QStringLiteral("教学楼条目格式无效"));
        }
        const QJsonObject object = value.toObject();
        const QJsonObject id = object.value(QStringLiteral("id")).toObject();
        ClassroomBuildingDto building{
            jsonString(id.value(QStringLiteral("campusNumber"))).trimmed(),
            jsonString(id.value(QStringLiteral("teachingBuildingNumber"))).trimmed(),
            object.value(QStringLiteral("teachingBuildingName")).toString().trimmed()
        };
        if (building.campusNumber.isEmpty() || building.teachingBuildingNumber.isEmpty()
            || building.teachingBuildingName.isEmpty()) {
            return failParse(errorMessage, QStringLiteral("教学楼条目缺少必要字段"));
        }
        parsed.buildings.append(building);
    }

    *result = parsed;
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

bool parseClassroomQuery(const QByteArray& body,
                         ClassroomQueryResultDto* result,
                         QString* errorMessage)
{
    if (!result) {
        return failParse(errorMessage, QStringLiteral("教室查询输出参数无效"));
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return failParse(errorMessage, QStringLiteral("教室查询响应不是有效 JSON 对象"));
    }

    const QJsonObject root = document.object();
    if (!root.value(QStringLiteral("classrooms")).isArray()
        || !root.value(QStringLiteral("classroomTime")).isArray()) {
        return failParse(errorMessage, QStringLiteral("教室查询响应缺少列表字段"));
    }

    ClassroomQueryResultDto parsed;
    parsed.date = root.value(QStringLiteral("date")).toString();
    parsed.teachingWeek = jsonInt(root.value(QStringLiteral("jxzc")));

    for (const QJsonValue& value : root.value(QStringLiteral("classrooms")).toArray()) {
        if (!value.isObject()) {
            return failParse(errorMessage, QStringLiteral("教室条目格式无效"));
        }
        const QJsonObject object = value.toObject();
        const QJsonObject id = object.value(QStringLiteral("id")).toObject();
        ClassroomInfoDto room;
        room.classroomName = object.value(QStringLiteral("classroomName")).toString().trimmed();
        room.classroomStatusCode = jsonString(object.value(QStringLiteral("classroomStatusCode")));
        room.classroomTypeCode = jsonString(object.value(QStringLiteral("classroomTypeCode")));
        room.campusNumber = jsonString(id.value(QStringLiteral("campusNumber"))).trimmed();
        room.classroomNumber = jsonString(id.value(QStringLiteral("classroomNumber"))).trimmed();
        room.teachingBuildingNumber = jsonString(id.value(QStringLiteral("teachingBuildingNumber"))).trimmed();
        room.placeNum = qMax(0, jsonInt(object.value(QStringLiteral("placeNum"))));
        room.remark = object.value(QStringLiteral("remark")).toString().trimmed();
        room.borrowable = object.value(QStringLiteral("sfkjy")).toString().trimmed();
        if (room.classroomName.isEmpty() || room.campusNumber.isEmpty()
            || room.classroomNumber.isEmpty() || room.teachingBuildingNumber.isEmpty()) {
            return failParse(errorMessage, QStringLiteral("教室条目缺少必要字段"));
        }
        parsed.classrooms.append(room);
    }

    for (const QJsonValue& value : root.value(QStringLiteral("classroomTime")).toArray()) {
        if (!value.isObject()) {
            return failParse(errorMessage, QStringLiteral("教室占用条目格式无效"));
        }
        const QJsonObject object = value.toObject();
        const QJsonObject id = object.value(QStringLiteral("id")).toObject();
        ClassroomTimeSlotDto slot;
        slot.campusNumber = jsonString(id.value(QStringLiteral("campusNumber"))).trimmed();
        slot.teachingBuildingNumber = jsonString(id.value(QStringLiteral("teachingBuildingNumber"))).trimmed();
        slot.classroomNumber = jsonString(id.value(QStringLiteral("classroomNumber"))).trimmed();
        slot.weekday = jsonInt(id.value(QStringLiteral("xq")));
        slot.sessionStart = jsonInt(id.value(QStringLiteral("sessionstart")));
        slot.continuingSession = jsonInt(object.value(QStringLiteral("continuingsession")), 1);
        slot.timeStateNumber = jsonString(object.value(QStringLiteral("timestatenumber")));
        slot.occupancyModuleId = jsonString(object.value(QStringLiteral("occupancymoduleId")));
        if (slot.campusNumber.isEmpty() || slot.teachingBuildingNumber.isEmpty()
            || slot.classroomNumber.isEmpty() || slot.sessionStart < 1 || slot.sessionStart > 12
            || slot.continuingSession < 1) {
            return failParse(errorMessage, QStringLiteral("教室占用条目缺少必要字段"));
        }
        parsed.timeSlots.append(slot);
    }

    *result = parsed;
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

// 提取培养方案成绩接口回调路径。
QString extractSchemeScoresCallback(const QString& html)
{
    return extractCallback(html, QStringLiteral("schemeScores"));
}

// 提取及格成绩接口回调路径。
QString extractPassingScoresCallback(const QString& html)
{
    return extractCallback(html, QStringLiteral("allPassingScores"));
}

}
