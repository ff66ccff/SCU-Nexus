#pragma once

#include "models/GradeModels.h"

#include <QObject>

class GradeStatisticsService : public QObject
{
    Q_OBJECT

public:
    explicit GradeStatisticsService(QObject *parent = nullptr);

    SchemeScoreSummary parseSchemeScores(const QJsonObject &root) const;
    PassingScoreResult parsePassingScores(const QJsonObject &root) const;
    QVariantMap customStats(const QList<GradeCourseItem> &items) const;
};
