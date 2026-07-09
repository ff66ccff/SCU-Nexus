#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

struct GradeCourseItem {
    QString courseName;
    QString englishCourseName;
    QString courseAttributeName;
    QString credit;
    QString rawScore;
    double courseScore = 0.0;
    double gradePointScore = 0.0;
    QString gradeName;
    QString academicYearCode;
    QString termName;
    bool passed = false;
    bool hasEffectiveScore = false;

    QString key() const;
    double creditValue() const;
    QVariantMap toVariant() const;
    static GradeCourseItem fromJson(const QJsonObject &object);
};

struct TermGradeGroup {
    QString label;
    QList<GradeCourseItem> items;

    QVariantMap toVariant() const;
};

struct SchemeScoreSummary {
    double totalCredits = 0.0;
    double earnedPlanCredits = 0.0;
    int passedCount = 0;
    int failedCount = 0;
    int totalCount = 0;
    QString scoreType;
    QList<GradeCourseItem> items;

    double gpa() const;
    double requiredGpa() const;
    double earnedCredits() const;
    double weightedAvgScore() const;
    double requiredWeightedAvgScore() const;
    double creditsByAttribute(const QString &attr) const;
    QList<TermGradeGroup> groupedByTerm() const;
    QVariantMap toSummaryVariant() const;
    static SchemeScoreSummary fromJson(const QJsonObject &root);
};

struct PassingScoreGroup {
    QString label;
    QList<GradeCourseItem> items;

    QVariantMap toVariant() const;
};

struct PassingScoreResult {
    QList<PassingScoreGroup> groups;

    double gpa() const;
    double totalCredits() const;
    int totalPassed() const;
    double weightedAvgScore() const;
    double requiredWeightedAvgScore() const;
    QVariantMap toSummaryVariant() const;
    static PassingScoreResult fromJson(const QJsonObject &root);
};

QList<TermGradeGroup> groupGradeCoursesByTerm(const QList<GradeCourseItem> &items);
QVariantList termGroupsToVariant(const QList<TermGradeGroup> &groups);
QVariantList passingGroupsToVariant(const QList<PassingScoreGroup> &groups);
QVariantMap customStatsForCourses(const QList<GradeCourseItem> &items);
