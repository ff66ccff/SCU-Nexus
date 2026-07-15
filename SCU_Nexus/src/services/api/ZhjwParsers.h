#ifndef ZHJWPARSERS_H
#define ZHJWPARSERS_H

#include "src/services/api/ApiDtos.h"

#include <QByteArray>
#include <QList>
#include <QString>

namespace ZhjwParsers {

// items 为空不一定是成功空结果：
// - explicitlyEmpty=true 表示页面明确写了“暂无数据”；
// - recognized=true 但 items 为空表示识别到卡片结构、关键字段却已改版；
// - 两者都为 false 表示拿到了完全无关的页面。
struct ExamPlanParseResult {
    QList<ExamPlanItemDto> items;
    bool recognized = false;
    bool explicitlyEmpty = false;
};

// 教务系统失效时可能返回 302、空 body 或 200 登录页，不能只看 HTTP 状态码。
bool isSessionExpired(const QString& body, int statusCode);
int parseCurrentWeek(const QString& html);
QList<SemesterDto> parseSemesters(const QString& html);
ExamPlanParseResult parseExamPlanResult(const QString& html);
QList<ExamPlanItemDto> parseExamPlan(const QString& html);
bool parseClassroomIndex(const QString& html,
                         ClassroomIndexDto* result,
                         QString* errorMessage = nullptr);
bool parseClassroomQuery(const QByteArray& body,
                         ClassroomQueryResultDto* result,
                         QString* errorMessage = nullptr);
// 成绩 callback 路径含服务端动态段，必须先访问入口页再提取，不能写死 URL。
QString extractSchemeScoresCallback(const QString& html);
QString extractPassingScoresCallback(const QString& html);

}

#endif // ZHJWPARSERS_H
