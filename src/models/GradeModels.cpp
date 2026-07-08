#include "models/GradeModels.h"

#include <algorithm>
#include <cmath>
#include <QMap>

namespace {
double rounded(double value)
{
    return std::round(value * 100.0) / 100.0;
}

QList<GradeCourseItem> effectiveCourses(const QList<GradeCourseItem> &items, bool requiredOnly = false)
{
    QList<GradeCourseItem> result;
    for (const GradeCourseItem &item : items) {
        if (!item.passed || !item.hasEffectiveScore || item.creditValue() <= 0.0) {
            continue;
        }
        if (requiredOnly && item.courseAttributeName != QStringLiteral("必修")) {
            continue;
        }
        result.append(item);
    }
    return result;
}

double weightedBy(const QList<GradeCourseItem> &items, bool useGpa, bool requiredOnly = false)
{
    const QList<GradeCourseItem> effective = effectiveCourses(items, requiredOnly);
    double sum = 0.0;
    double credits = 0.0;
    for (const GradeCourseItem &item : effective) {
        const double credit = item.creditValue();
        sum += (useGpa ? item.gradePointScore : item.courseScore) * credit;
        credits += credit;
    }
    return credits > 0.0 ? rounded(sum / credits) : 0.0;
}

int termRank(const QString &label)
{
    if (label.contains(QStringLiteral("春"))) {
        return 2;
    }
    if (label.contains(QStringLiteral("秋"))) {
        return 1;
    }
    return 0;
}
}

QString GradeCourseItem::key() const
{
    return courseName + QStringLiteral("|") + academicYearCode + QStringLiteral("|") + termName + QStringLiteral("|") + courseAttributeName;
}

double GradeCourseItem::creditValue() const
{
    bool ok = false;
    const double value = credit.toDouble(&ok);
    return ok ? value : 0.0;
}

QVariantMap GradeCourseItem::toVariant() const
{
    return {
        {QStringLiteral("key"), key()},
        {QStringLiteral("courseName"), courseName},
        {QStringLiteral("englishCourseName"), englishCourseName},
        {QStringLiteral("courseAttributeName"), courseAttributeName},
        {QStringLiteral("credit"), credit},
        {QStringLiteral("rawScore"), rawScore},
        {QStringLiteral("courseScore"), courseScore},
        {QStringLiteral("gradePointScore"), gradePointScore},
        {QStringLiteral("gradeName"), gradeName},
        {QStringLiteral("academicYearCode"), academicYearCode},
        {QStringLiteral("termName"), termName},
        {QStringLiteral("passed"), passed},
        {QStringLiteral("hasEffectiveScore"), hasEffectiveScore}
    };
}

GradeCourseItem GradeCourseItem::fromJson(const QJsonObject &object)
{
    GradeCourseItem item;
    item.courseName = object.value(QStringLiteral("courseName")).toString();
    item.englishCourseName = object.value(QStringLiteral("englishCourseName")).toString();
    item.courseAttributeName = object.value(QStringLiteral("courseAttributeName")).toString();
    item.credit = object.value(QStringLiteral("credit")).toString();
    item.rawScore = object.value(QStringLiteral("cj")).toString();
    item.courseScore = object.value(QStringLiteral("courseScore")).toDouble(-1.0);
    item.gradePointScore = object.value(QStringLiteral("gradePointScore")).toDouble(-1.0);
    item.gradeName = object.value(QStringLiteral("gradeName")).toString();
    item.academicYearCode = object.value(QStringLiteral("academicYearCode")).toString();
    item.termName = object.value(QStringLiteral("termName")).toString();
    item.passed = item.gradeName != QStringLiteral("F") && !item.gradeName.isEmpty();
    item.hasEffectiveScore = item.courseScore >= 0.0 && item.gradePointScore >= 0.0;
    return item;
}

QVariantMap TermGradeGroup::toVariant() const
{
    QVariantList courseMaps;
    double credits = 0.0;
    int passed = 0;
    for (const GradeCourseItem &item : items) {
        courseMaps.append(item.toVariant());
        if (item.passed) {
            credits += item.creditValue();
            passed += 1;
        }
    }
    return {
        {QStringLiteral("label"), label},
        {QStringLiteral("items"), courseMaps},
        {QStringLiteral("credits"), rounded(credits)},
        {QStringLiteral("passedCount"), passed}
    };
}

double SchemeScoreSummary::gpa() const
{
    return weightedBy(items, true);
}

double SchemeScoreSummary::requiredGpa() const
{
    return weightedBy(items, true, true);
}

double SchemeScoreSummary::earnedCredits() const
{
    double sum = 0.0;
    for (const GradeCourseItem &item : items) {
        if (item.passed) {
            sum += item.creditValue();
        }
    }
    return rounded(sum);
}

double SchemeScoreSummary::weightedAvgScore() const
{
    return weightedBy(items, false);
}

double SchemeScoreSummary::requiredWeightedAvgScore() const
{
    return weightedBy(items, false, true);
}

double SchemeScoreSummary::creditsByAttribute(const QString &attr) const
{
    double sum = 0.0;
    for (const GradeCourseItem &item : items) {
        if (item.passed && item.courseAttributeName == attr) {
            sum += item.creditValue();
        }
    }
    return rounded(sum);
}

QList<TermGradeGroup> SchemeScoreSummary::groupedByTerm() const
{
    return groupGradeCoursesByTerm(items);
}

QVariantMap SchemeScoreSummary::toSummaryVariant() const
{
    return {
        {QStringLiteral("gpa"), gpa()},
        {QStringLiteral("requiredGpa"), requiredGpa()},
        {QStringLiteral("passedCount"), passedCount},
        {QStringLiteral("failedCount"), failedCount},
        {QStringLiteral("totalCount"), totalCount},
        {QStringLiteral("weightedAvgScore"), weightedAvgScore()},
        {QStringLiteral("requiredWeightedAvgScore"), requiredWeightedAvgScore()},
        {QStringLiteral("earnedCredits"), earnedCredits()},
        {QStringLiteral("totalCredits"), totalCredits},
        {QStringLiteral("earnedPlanCredits"), earnedPlanCredits},
        {QStringLiteral("electiveCredits"), creditsByAttribute(QStringLiteral("选修"))},
        {QStringLiteral("optionalCredits"), creditsByAttribute(QStringLiteral("任选"))},
        {QStringLiteral("scoreType"), scoreType}
    };
}

SchemeScoreSummary SchemeScoreSummary::fromJson(const QJsonObject &root)
{
    SchemeScoreSummary summary;
    const QJsonArray years = root.value(QStringLiteral("lnList")).toArray();
    if (years.isEmpty() || !years.first().isObject()) {
        return summary;
    }

    const QJsonObject first = years.first().toObject();
    summary.totalCredits = first.value(QStringLiteral("zxf")).toDouble();
    summary.earnedPlanCredits = first.value(QStringLiteral("yxxf")).toDouble();
    summary.passedCount = first.value(QStringLiteral("tgms")).toInt();
    summary.failedCount = first.value(QStringLiteral("wtgms")).toInt();
    summary.totalCount = first.value(QStringLiteral("zms")).toInt();
    summary.scoreType = first.value(QStringLiteral("cjlx")).toString();

    const QJsonArray courses = first.value(QStringLiteral("cjList")).toArray();
    for (const QJsonValue &value : courses) {
        if (value.isObject()) {
            summary.items.append(GradeCourseItem::fromJson(value.toObject()));
        }
    }

    if (summary.totalCount == 0) {
        summary.totalCount = summary.items.size();
    }
    if (summary.passedCount == 0) {
        for (const GradeCourseItem &item : summary.items) {
            summary.passedCount += item.passed ? 1 : 0;
        }
        summary.failedCount = summary.totalCount - summary.passedCount;
    }
    return summary;
}

QVariantMap PassingScoreGroup::toVariant() const
{
    QVariantList courseMaps;
    for (const GradeCourseItem &item : items) {
        courseMaps.append(item.toVariant());
    }
    return {
        {QStringLiteral("label"), label},
        {QStringLiteral("items"), courseMaps},
        {QStringLiteral("passedCount"), items.size()},
        {QStringLiteral("credits"), customStatsForCourses(items).value(QStringLiteral("credits"))}
    };
}

double PassingScoreResult::gpa() const
{
    QList<GradeCourseItem> all;
    for (const PassingScoreGroup &group : groups) {
        all.append(group.items);
    }
    return weightedBy(all, true);
}

double PassingScoreResult::totalCredits() const
{
    QList<GradeCourseItem> all;
    for (const PassingScoreGroup &group : groups) {
        all.append(group.items);
    }
    return customStatsForCourses(all).value(QStringLiteral("credits")).toDouble();
}

int PassingScoreResult::totalPassed() const
{
    int count = 0;
    for (const PassingScoreGroup &group : groups) {
        count += group.items.size();
    }
    return count;
}

double PassingScoreResult::weightedAvgScore() const
{
    QList<GradeCourseItem> all;
    for (const PassingScoreGroup &group : groups) {
        all.append(group.items);
    }
    return weightedBy(all, false);
}

double PassingScoreResult::requiredWeightedAvgScore() const
{
    QList<GradeCourseItem> all;
    for (const PassingScoreGroup &group : groups) {
        all.append(group.items);
    }
    return weightedBy(all, false, true);
}

QVariantMap PassingScoreResult::toSummaryVariant() const
{
    return {
        {QStringLiteral("gpa"), gpa()},
        {QStringLiteral("totalCredits"), totalCredits()},
        {QStringLiteral("totalPassed"), totalPassed()},
        {QStringLiteral("termCount"), groups.size()},
        {QStringLiteral("weightedAvgScore"), weightedAvgScore()},
        {QStringLiteral("requiredWeightedAvgScore"), requiredWeightedAvgScore()}
    };
}

PassingScoreResult PassingScoreResult::fromJson(const QJsonObject &root)
{
    PassingScoreResult result;
    const QJsonArray years = root.value(QStringLiteral("lnList")).toArray();
    for (const QJsonValue &value : years) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject object = value.toObject();
        PassingScoreGroup group;
        group.label = object.value(QStringLiteral("cjlx")).toString();
        const QJsonArray courses = object.value(QStringLiteral("cjList")).toArray();
        for (const QJsonValue &courseValue : courses) {
            if (courseValue.isObject()) {
                group.items.append(GradeCourseItem::fromJson(courseValue.toObject()));
            }
        }
        result.groups.append(group);
    }

    std::sort(result.groups.begin(), result.groups.end(), [](const PassingScoreGroup &left, const PassingScoreGroup &right) {
        const QString leftYear = left.label.left(9);
        const QString rightYear = right.label.left(9);
        if (leftYear != rightYear) {
            return leftYear > rightYear;
        }
        return termRank(left.label) > termRank(right.label);
    });
    return result;
}

QList<TermGradeGroup> groupGradeCoursesByTerm(const QList<GradeCourseItem> &items)
{
    QMap<QString, QList<GradeCourseItem>> groups;
    for (const GradeCourseItem &item : items) {
        const QString label = item.academicYearCode + QStringLiteral(" ") + item.termName;
        groups[label].append(item);
    }

    QList<TermGradeGroup> result;
    for (auto it = groups.cbegin(); it != groups.cend(); ++it) {
        result.append({it.key(), it.value()});
    }
    std::sort(result.begin(), result.end(), [](const TermGradeGroup &left, const TermGradeGroup &right) {
        if (left.label.left(9) != right.label.left(9)) {
            return left.label.left(9) > right.label.left(9);
        }
        return termRank(left.label) > termRank(right.label);
    });
    return result;
}

QVariantList termGroupsToVariant(const QList<TermGradeGroup> &groups)
{
    QVariantList list;
    for (const TermGradeGroup &group : groups) {
        list.append(group.toVariant());
    }
    return list;
}

QVariantList passingGroupsToVariant(const QList<PassingScoreGroup> &groups)
{
    QVariantList list;
    for (const PassingScoreGroup &group : groups) {
        list.append(group.toVariant());
    }
    return list;
}

QVariantMap customStatsForCourses(const QList<GradeCourseItem> &items)
{
    int passed = 0;
    int failed = 0;
    double credits = 0.0;
    for (const GradeCourseItem &item : items) {
        if (item.passed) {
            passed += 1;
            credits += item.creditValue();
        } else {
            failed += 1;
        }
    }
    return {
        {QStringLiteral("gpa"), weightedBy(items, true)},
        {QStringLiteral("credits"), rounded(credits)},
        {QStringLiteral("weightedAvgScore"), weightedBy(items, false)},
        {QStringLiteral("requiredWeightedAvgScore"), weightedBy(items, false, true)},
        {QStringLiteral("passedCount"), passed},
        {QStringLiteral("failedCount"), failed},
        {QStringLiteral("selectedCount"), items.size()}
    };
}
