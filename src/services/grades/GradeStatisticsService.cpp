#include "services/grades/GradeStatisticsService.h"

GradeStatisticsService::GradeStatisticsService(QObject *parent)
    : QObject(parent)
{
}

SchemeScoreSummary GradeStatisticsService::parseSchemeScores(const QJsonObject &root) const
{
    return SchemeScoreSummary::fromJson(root);
}

PassingScoreResult GradeStatisticsService::parsePassingScores(const QJsonObject &root) const
{
    return PassingScoreResult::fromJson(root);
}

QVariantMap GradeStatisticsService::customStats(const QList<GradeCourseItem> &items) const
{
    return customStatsForCourses(items);
}
