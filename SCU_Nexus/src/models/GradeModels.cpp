#include "models/GradeModels.h"

#include <algorithm>
#include <cmath>
#include <QMap>

namespace {
// 将统计结果四舍五入到两位小数。
double rounded(double value)
{
    return std::round(value * 100.0) / 100.0;
}

// 筛选可计入绩点和均分统计的有效课程。
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

// 按学分加权计算绩点或成绩均值。
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

// 将学期文本转换为用于排序的季节权重。
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

// 生成课程在界面选择中的稳定唯一键。
QString GradeCourseItem::key() const
{
    return courseName + QStringLiteral("|") + academicYearCode + QStringLiteral("|") + termName + QStringLiteral("|") + courseAttributeName;
}

// 将课程学分文本转换为可计算的数值。
double GradeCourseItem::creditValue() const
{
    bool ok = false;
    const double value = credit.toDouble(&ok);
    return ok ? value : 0.0;
}

// 转换为界面可直接读取的 QVariant 数据。
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

// 从 JSON 对象还原数据模型。
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

// 转换为界面可直接读取的 QVariant 数据。
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

// 计算通过课程的平均绩点。
double SchemeScoreSummary::gpa() const
{
    return weightedBy(items, true);
}

// 计算必修课程的平均绩点。
double SchemeScoreSummary::requiredGpa() const
{
    return weightedBy(items, true, true);
}

// 汇总已通过课程获得的学分。
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

// 计算通过课程的学分加权平均分。
double SchemeScoreSummary::weightedAvgScore() const
{
    return weightedBy(items, false);
}

// 计算必修通过课程的学分加权平均分。
double SchemeScoreSummary::requiredWeightedAvgScore() const
{
    return weightedBy(items, false, true);
}

// 按课程属性汇总已通过课程学分。
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

// 按业务维度分组数据并返回结果。
QList<TermGradeGroup> SchemeScoreSummary::groupedByTerm() const
{
    return groupGradeCoursesByTerm(items);
}

// 汇总统计指标为界面可绑定的 QVariantMap。
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

// 从 JSON 对象还原数据模型。
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

// 转换为界面可直接读取的 QVariant 数据。
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

// 计算通过课程的平均绩点。
double PassingScoreResult::gpa() const
{
    QList<GradeCourseItem> all;
    for (const PassingScoreGroup &group : groups) {
        all.append(group.items);
    }
    return weightedBy(all, true);
}

// 汇总及格成绩结果中的课程学分。
double PassingScoreResult::totalCredits() const
{
    QList<GradeCourseItem> all;
    for (const PassingScoreGroup &group : groups) {
        all.append(group.items);
    }
    return customStatsForCourses(all).value(QStringLiteral("credits")).toDouble();
}

// 汇总及格成绩结果中的通过课程数量。
int PassingScoreResult::totalPassed() const
{
    int count = 0;
    for (const PassingScoreGroup &group : groups) {
        count += group.items.size();
    }
    return count;
}

// 计算通过课程的学分加权平均分。
double PassingScoreResult::weightedAvgScore() const
{
    QList<GradeCourseItem> all;
    for (const PassingScoreGroup &group : groups) {
        all.append(group.items);
    }
    return weightedBy(all, false);
}

// 计算必修通过课程的学分加权平均分。
double PassingScoreResult::requiredWeightedAvgScore() const
{
    QList<GradeCourseItem> all;
    for (const PassingScoreGroup &group : groups) {
        all.append(group.items);
    }
    return weightedBy(all, false, true);
}

// 汇总统计指标为界面可绑定的 QVariantMap。
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

// 从 JSON 对象还原数据模型。
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

// 按业务维度分组数据并返回结果。
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

// 将学期分组列表转换为界面可绑定数据。
QVariantList termGroupsToVariant(const QList<TermGradeGroup> &groups)
{
    QVariantList list;
    for (const TermGradeGroup &group : groups) {
        list.append(group.toVariant());
    }
    return list;
}

// 将及格成绩分组列表转换为界面可绑定数据。
QVariantList passingGroupsToVariant(const QList<PassingScoreGroup> &groups)
{
    QVariantList list;
    for (const PassingScoreGroup &group : groups) {
        list.append(group.toVariant());
    }
    return list;
}

// 基于自定义课程集合计算统计信息。
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
