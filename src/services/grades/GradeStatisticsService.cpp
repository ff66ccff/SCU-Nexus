#include "services/grades/GradeStatisticsService.h"

// 构造对象并初始化依赖关系。
GradeStatisticsService::GradeStatisticsService(QObject *parent)
    : QObject(parent)
{
}

// 解析外部数据并转换为内部结构。
SchemeScoreSummary GradeStatisticsService::parseSchemeScores(const QJsonObject &root) const
{
    return SchemeScoreSummary::fromJson(root);
}

// 解析外部数据并转换为内部结构。
PassingScoreResult GradeStatisticsService::parsePassingScores(const QJsonObject &root) const
{
    return PassingScoreResult::fromJson(root);
}

// 基于自定义课程集合计算统计信息。
QVariantMap GradeStatisticsService::customStats(const QList<GradeCourseItem> &items) const
{
    return customStatsForCourses(items);
}
