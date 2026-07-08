#include "models/AcademicCalendarModels.h"
#include "models/ExamPlanModels.h"
#include "models/GradeModels.h"
#include "services/calendar/AcademicCalendarService.h"
#include "services/grades/GradeStatisticsService.h"
#include "common/QueryState.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryFile>
#include <QtTest>

class DModuleTest : public QObject
{
    Q_OBJECT

private slots:
    // --- Calendar: parsing ---
    void parsesCalendarEntries();
    void parsesCalendarEntryEmptyHtml();
    void parsesCalendarImages();
    void parsesCalendarImagesEmpty();
    void decodesGb18030Fallback();

    // --- Exam: isPast ---
    void isPastWithValidDateTime();
    void isPastWithDateOnlyFallback();
    void isPastWithParseFailure();

    // --- Exam: sorting ---
    void sortsByStartTime();
    void sortsUnparseableTimeAtEnd();
    void sortsEmptyList();

    // --- Grade: field mapping ---
    void mapsGradeCourseItemFields();
    void detectsPassedAndEffectiveScore();
    void gradeCourseItemKey();

    // --- Grade: Scheme scores ---
    void parsesSchemeScoresAndCalculatesStats();
    void parsesSchemeScoresWithEmptyItems();
    void calculatesRequiredGpa();
    void calculatesPassedCountFromItems();

    // --- Grade: Passing scores ---
    void parsesPassingScoresJson();
    void sortsPassingScores();
    void calculatesPassingStats();

    // --- Grade: Custom stats ---
    void customStatsForSelectedCourses();
    void customStatsForEmptyList();

    // --- Grade: Term grouping ---
    void groupsByTermAndSorts();

    // --- Grade: Search filtering ---
    void creditsByAttribute();

    // --- GradeStatisticsService ---
    void gradeStatisticsServiceWrapsParsing();
};

// ---------------------------------------------------------------------------
// Calendar: parsing
// ---------------------------------------------------------------------------

void DModuleTest::parsesCalendarEntries()
{
    const QString html = QStringLiteral(R"(
        <a href="info/1101/1234.htm">四川大学 2025-2026 学年校历</a>
        <a href="info/1101/1235.htm">四川大学 2024-2025 学年校历</a>
    )");
    const QList<AcademicCalendarEntry> entries = AcademicCalendarService::parseEntries(html);
    QCOMPARE(entries.size(), 2);
    QCOMPARE(entries.first().title, QStringLiteral("2025-2026"));
    QCOMPARE(entries.first().path, QStringLiteral("info/1101/1234.htm"));
}

void DModuleTest::parsesCalendarEntryEmptyHtml()
{
    const QList<AcademicCalendarEntry> entries = AcademicCalendarService::parseEntries(
        QStringLiteral("<html><body>No links here</body></html>"));
    QVERIFY(entries.isEmpty());
}

void DModuleTest::parsesCalendarImages()
{
    const QString html = QStringLiteral(R"(
        <img src="/__local/AB/CD/calendar.jpg">
        <img src="/__local/EF/GH/calendar.png">
    )");
    const QStringList urls = AcademicCalendarService::parseImageUrls(html);
    QCOMPARE(urls.size(), 2);
    QVERIFY(urls.first().startsWith(QStringLiteral("https://jwc.scu.edu.cn/__local/")));
}

void DModuleTest::parsesCalendarImagesEmpty()
{
    const QStringList urls = AcademicCalendarService::parseImageUrls(
        QStringLiteral("<html><body>No images</body></html>"));
    QVERIFY(urls.isEmpty());
}

void DModuleTest::decodesGb18030Fallback()
{
    const QString decoded = AcademicCalendarService::decodeHtml(
        QByteArray("\xbf\xd5\xbc\xe4\xd6\xd0\xce\xc4"), QByteArray());
    QVERIFY(!decoded.isEmpty());
    QVERIFY(!decoded.contains(QChar::ReplacementCharacter));
}

// ---------------------------------------------------------------------------
// Exam: isPast
// ---------------------------------------------------------------------------

void DModuleTest::isPastWithValidDateTime()
{
    ExamPlanItem item;
    item.courseName = QStringLiteral("B");
    item.date = QStringLiteral("2026-01-10");
    item.timeRange = QStringLiteral("14:00-16:00");
    updateExamTemporalFields(&item, QDateTime(QDate(2026, 1, 10), QTime(17, 0)));
    QVERIFY(item.isPast);
    QVERIFY(item.startDateTime.isValid());
    QVERIFY(item.endDateTime.isValid());

    ExamPlanItem early;
    early.courseName = QStringLiteral("A");
    early.date = QStringLiteral("2026-01-10");
    early.timeRange = QStringLiteral("09:00-11:00");
    updateExamTemporalFields(&early, QDateTime(QDate(2026, 1, 1), QTime(8, 0)));
    QVERIFY(!early.isPast);
}

void DModuleTest::isPastWithDateOnlyFallback()
{
    ExamPlanItem item;
    item.date = QStringLiteral("2026-01-10");
    item.timeRange = QStringLiteral("morning");
    updateExamTemporalFields(&item, QDateTime(QDate(2026, 1, 12), QTime(8, 0)));
    QVERIFY(item.isPast);

    ExamPlanItem future;
    future.date = QStringLiteral("2026-01-10");
    future.timeRange = QStringLiteral("morning");
    updateExamTemporalFields(&future, QDateTime(QDate(2026, 1, 9), QTime(8, 0)));
    QVERIFY(!future.isPast);
}

void DModuleTest::isPastWithParseFailure()
{
    ExamPlanItem item;
    item.date = QStringLiteral("TBD");
    item.timeRange = QString();
    updateExamTemporalFields(&item, QDateTime::currentDateTime());
    QVERIFY(!item.isPast);
    QVERIFY(!item.startDateTime.isValid());
    QVERIFY(!item.endDateTime.isValid());
}

// ---------------------------------------------------------------------------
// Exam: sorting
// ---------------------------------------------------------------------------

void DModuleTest::sortsByStartTime()
{
    ExamPlanItem late;
    late.courseName = QStringLiteral("B");
    late.date = QStringLiteral("2026-01-10");
    late.timeRange = QStringLiteral("14:00-16:00");
    updateExamTemporalFields(&late, QDateTime(QDate(2026, 1, 1), QTime(8, 0)));

    ExamPlanItem early;
    early.courseName = QStringLiteral("A");
    early.date = QStringLiteral("2026-01-10");
    early.timeRange = QStringLiteral("09:00-11:00");
    updateExamTemporalFields(&early, QDateTime(QDate(2026, 1, 1), QTime(8, 0)));

    QList<ExamPlanItem> items{late, early};
    sortExamPlanItems(&items);
    QCOMPARE(items.first().courseName, QStringLiteral("A"));
}

void DModuleTest::sortsUnparseableTimeAtEnd()
{
    ExamPlanItem parsed;
    parsed.courseName = QStringLiteral("Z");
    parsed.date = QStringLiteral("2026-01-10");
    parsed.timeRange = QStringLiteral("09:00-11:00");
    updateExamTemporalFields(&parsed);

    ExamPlanItem unparsed;
    unparsed.courseName = QStringLiteral("A");
    unparsed.date = QStringLiteral("??");
    unparsed.timeRange = QString();
    updateExamTemporalFields(&unparsed);

    QList<ExamPlanItem> items{unparsed, parsed};
    sortExamPlanItems(&items);
    QCOMPARE(items.first().courseName, QStringLiteral("Z"));
    QCOMPARE(items.last().courseName, QStringLiteral("A"));
}

void DModuleTest::sortsEmptyList()
{
    QList<ExamPlanItem> items;
    sortExamPlanItems(&items);
    QVERIFY(items.isEmpty());
}

// ---------------------------------------------------------------------------
// Grade: field mapping
// ---------------------------------------------------------------------------

void DModuleTest::mapsGradeCourseItemFields()
{
    QJsonObject obj{
        {QStringLiteral("courseName"), QStringLiteral("高等数学")},
        {QStringLiteral("englishCourseName"), QStringLiteral("Advanced Math")},
        {QStringLiteral("courseAttributeName"), QStringLiteral("必修")},
        {QStringLiteral("credit"), QStringLiteral("5")},
        {QStringLiteral("cj"), QStringLiteral("91")},
        {QStringLiteral("courseScore"), 91.0},
        {QStringLiteral("gradePointScore"), 4.0},
        {QStringLiteral("gradeName"), QStringLiteral("A")},
        {QStringLiteral("academicYearCode"), QStringLiteral("2025-2026")},
        {QStringLiteral("termName"), QStringLiteral("秋")}
    };

    GradeCourseItem item = GradeCourseItem::fromJson(obj);
    QCOMPARE(item.courseName, QStringLiteral("高等数学"));
    QCOMPARE(item.englishCourseName, QStringLiteral("Advanced Math"));
    QCOMPARE(item.courseAttributeName, QStringLiteral("必修"));
    QCOMPARE(item.credit, QStringLiteral("5"));
    QCOMPARE(item.rawScore, QStringLiteral("91"));
    QCOMPARE(item.courseScore, 91.0);
    QCOMPARE(item.gradePointScore, 4.0);
    QCOMPARE(item.gradeName, QStringLiteral("A"));
    QCOMPARE(item.academicYearCode, QStringLiteral("2025-2026"));
    QCOMPARE(item.termName, QStringLiteral("秋"));
}

void DModuleTest::detectsPassedAndEffectiveScore()
{
    GradeCourseItem passed;
    passed.gradeName = QStringLiteral("A");
    passed.courseScore = 90.0;
    passed.gradePointScore = 4.0;
    passed = GradeCourseItem::fromJson(QJsonObject{
        {QStringLiteral("gradeName"), QStringLiteral("A")},
        {QStringLiteral("courseScore"), 90.0},
        {QStringLiteral("gradePointScore"), 4.0}
    });
    QVERIFY(passed.passed);
    QVERIFY(passed.hasEffectiveScore);

    GradeCourseItem failed;
    failed = GradeCourseItem::fromJson(QJsonObject{
        {QStringLiteral("gradeName"), QStringLiteral("F")},
        {QStringLiteral("courseScore"), 55.0},
        {QStringLiteral("gradePointScore"), 0.0}
    });
    QVERIFY(!failed.passed);
    QVERIFY(failed.hasEffectiveScore);

    GradeCourseItem noScore;
    noScore = GradeCourseItem::fromJson(QJsonObject{
        {QStringLiteral("gradeName"), QStringLiteral("A")},
        {QStringLiteral("courseScore"), -1.0},
        {QStringLiteral("gradePointScore"), -1.0}
    });
    QVERIFY(noScore.passed);
    QVERIFY(!noScore.hasEffectiveScore);

    GradeCourseItem emptyGrade;
    emptyGrade = GradeCourseItem::fromJson(QJsonObject{});
    QVERIFY(!emptyGrade.passed);
    QVERIFY(emptyGrade.gradeName.isEmpty());
}

void DModuleTest::gradeCourseItemKey()
{
    GradeCourseItem item;
    item.courseName = QStringLiteral("Math");
    item.academicYearCode = QStringLiteral("2025-2026");
    item.termName = QStringLiteral("秋");
    item.courseAttributeName = QStringLiteral("必修");
    QCOMPARE(item.key(), QStringLiteral("Math|2025-2026|秋|必修"));
}

// ---------------------------------------------------------------------------
// Grade: Scheme scores
// ---------------------------------------------------------------------------

void DModuleTest::parsesSchemeScoresAndCalculatesStats()
{
    const QJsonObject courseA{
        {QStringLiteral("courseName"), QStringLiteral("A")},
        {QStringLiteral("courseAttributeName"), QStringLiteral("必修")},
        {QStringLiteral("credit"), QStringLiteral("2")},
        {QStringLiteral("cj"), QStringLiteral("90")},
        {QStringLiteral("courseScore"), 90.0},
        {QStringLiteral("gradePointScore"), 4.0},
        {QStringLiteral("gradeName"), QStringLiteral("A")},
        {QStringLiteral("academicYearCode"), QStringLiteral("2025-2026")},
        {QStringLiteral("termName"), QStringLiteral("秋")}
    };
    const QJsonObject courseB{
        {QStringLiteral("courseName"), QStringLiteral("B")},
        {QStringLiteral("courseAttributeName"), QStringLiteral("选修")},
        {QStringLiteral("credit"), QStringLiteral("2")},
        {QStringLiteral("cj"), QStringLiteral("80")},
        {QStringLiteral("courseScore"), 80.0},
        {QStringLiteral("gradePointScore"), 3.0},
        {QStringLiteral("gradeName"), QStringLiteral("B")},
        {QStringLiteral("academicYearCode"), QStringLiteral("2025-2026")},
        {QStringLiteral("termName"), QStringLiteral("春")}
    };
    const QJsonObject root{
        {QStringLiteral("lnList"), QJsonArray{
            QJsonObject{
                {QStringLiteral("zxf"), 160.0},
                {QStringLiteral("yxxf"), 4.0},
                {QStringLiteral("cjlx"), QStringLiteral("方案成绩")},
                {QStringLiteral("cjList"), QJsonArray{courseA, courseB}}
            }
        }}
    };

    const SchemeScoreSummary summary = SchemeScoreSummary::fromJson(root);
    QCOMPARE(summary.items.size(), 2);
    QCOMPARE(summary.totalCredits, 160.0);
    QCOMPARE(summary.earnedPlanCredits, 4.0);
    QCOMPARE(summary.scoreType, QStringLiteral("方案成绩"));
    QCOMPARE(summary.gpa(), 3.5);
    QCOMPARE(summary.requiredGpa(), 4.0);
    QCOMPARE(summary.weightedAvgScore(), 85.0);
}

void DModuleTest::parsesSchemeScoresWithEmptyItems()
{
    const QJsonObject root{
        {QStringLiteral("lnList"), QJsonArray{}}
    };
    const SchemeScoreSummary summary = SchemeScoreSummary::fromJson(root);
    QVERIFY(summary.items.isEmpty());
    QCOMPARE(summary.gpa(), 0.0);
}

void DModuleTest::calculatesRequiredGpa()
{
    QJsonObject courseA{
        {QStringLiteral("courseName"), QStringLiteral("必修课")},
        {QStringLiteral("courseAttributeName"), QStringLiteral("必修")},
        {QStringLiteral("credit"), QStringLiteral("3")},
        {QStringLiteral("courseScore"), 90.0},
        {QStringLiteral("gradePointScore"), 4.0},
        {QStringLiteral("gradeName"), QStringLiteral("A")}
    };
    QJsonObject courseB{
        {QStringLiteral("courseName"), QStringLiteral("选修课")},
        {QStringLiteral("courseAttributeName"), QStringLiteral("选修")},
        {QStringLiteral("credit"), QStringLiteral("3")},
        {QStringLiteral("courseScore"), 80.0},
        {QStringLiteral("gradePointScore"), 0.0},
        {QStringLiteral("gradeName"), QStringLiteral("B")}
    };
    const QJsonObject root{
        {QStringLiteral("lnList"), QJsonArray{
            QJsonObject{{QStringLiteral("cjList"), QJsonArray{courseA, courseB}}}
        }}
    };
    const SchemeScoreSummary summary = SchemeScoreSummary::fromJson(root);
    QCOMPARE(summary.requiredGpa(), 4.0);
    QCOMPARE(summary.requiredWeightedAvgScore(), 90.0);
}

void DModuleTest::calculatesPassedCountFromItems()
{
    QJsonObject passed{
        {QStringLiteral("courseName"), QStringLiteral("P")},
        {QStringLiteral("courseScore"), 85.0},
        {QStringLiteral("gradePointScore"), 3.5},
        {QStringLiteral("gradeName"), QStringLiteral("B+")},
        {QStringLiteral("credit"), QStringLiteral("2")},
        {QStringLiteral("courseAttributeName"), QStringLiteral("必修")}
    };
    QJsonObject failed{
        {QStringLiteral("courseName"), QStringLiteral("F")},
        {QStringLiteral("courseScore"), 55.0},
        {QStringLiteral("gradePointScore"), 0.0},
        {QStringLiteral("gradeName"), QStringLiteral("F")},
        {QStringLiteral("credit"), QStringLiteral("2")},
        {QStringLiteral("courseAttributeName"), QStringLiteral("必修")}
    };
    const QJsonObject root{
        {QStringLiteral("lnList"), QJsonArray{
            QJsonObject{
                {QStringLiteral("zms"), 2},
                {QStringLiteral("cjList"), QJsonArray{passed, failed}}
            }
        }}
    };
    const SchemeScoreSummary summary = SchemeScoreSummary::fromJson(root);
    QCOMPARE(summary.totalCount, 2);
    QCOMPARE(summary.passedCount, 1);
    QCOMPARE(summary.failedCount, 1);
}

// ---------------------------------------------------------------------------
// Grade: Passing scores
// ---------------------------------------------------------------------------

void DModuleTest::parsesPassingScoresJson()
{
    const QJsonObject course{
        {QStringLiteral("courseName"), QStringLiteral("P")},
        {QStringLiteral("courseScore"), 85.0},
        {QStringLiteral("gradePointScore"), 3.5},
        {QStringLiteral("gradeName"), QStringLiteral("B+")},
        {QStringLiteral("credit"), QStringLiteral("2")},
        {QStringLiteral("courseAttributeName"), QStringLiteral("必修")}
    };
    const QJsonObject root{
        {QStringLiteral("lnList"), QJsonArray{
            QJsonObject{
                {QStringLiteral("cjlx"), QStringLiteral("2025-2026学年秋(两学期)")},
                {QStringLiteral("cjList"), QJsonArray{course}}
            }
        }}
    };
    const PassingScoreResult result = PassingScoreResult::fromJson(root);
    QCOMPARE(result.groups.size(), 1);
    QCOMPARE(result.groups.first().label, QStringLiteral("2025-2026学年秋(两学期)"));
    QCOMPARE(result.groups.first().items.size(), 1);
    QCOMPARE(result.totalPassed(), 1);
    QVERIFY(result.gpa() > 0.0);
}

void DModuleTest::sortsPassingScores()
{
    const QJsonObject root{
        {QStringLiteral("lnList"), QJsonArray{
            QJsonObject{{QStringLiteral("cjlx"), QStringLiteral("2024-2025学年秋")}, {QStringLiteral("cjList"), QJsonArray{}}},
            QJsonObject{{QStringLiteral("cjlx"), QStringLiteral("2025-2026学年秋")}, {QStringLiteral("cjList"), QJsonArray{}}},
            QJsonObject{{QStringLiteral("cjlx"), QStringLiteral("2025-2026学年春")}, {QStringLiteral("cjList"), QJsonArray{}}}
        }}
    };
    const PassingScoreResult result = PassingScoreResult::fromJson(root);
    QCOMPARE(result.groups.size(), 3);
    QCOMPARE(result.groups.first().label, QStringLiteral("2025-2026学年春"));
    QCOMPARE(result.groups.at(1).label, QStringLiteral("2025-2026学年秋"));
    QCOMPARE(result.groups.last().label, QStringLiteral("2024-2025学年秋"));
}

void DModuleTest::calculatesPassingStats()
{
    const QJsonObject courseA{
        {QStringLiteral("courseName"), QStringLiteral("A")},
        {QStringLiteral("credit"), QStringLiteral("3")},
        {QStringLiteral("courseScore"), 90.0},
        {QStringLiteral("gradePointScore"), 4.0},
        {QStringLiteral("gradeName"), QStringLiteral("A")},
        {QStringLiteral("courseAttributeName"), QStringLiteral("必修")}
    };
    const QJsonObject courseB{
        {QStringLiteral("courseName"), QStringLiteral("B")},
        {QStringLiteral("credit"), QStringLiteral("2")},
        {QStringLiteral("courseScore"), 80.0},
        {QStringLiteral("gradePointScore"), 3.0},
        {QStringLiteral("gradeName"), QStringLiteral("B")},
        {QStringLiteral("courseAttributeName"), QStringLiteral("必修")}
    };
    const QJsonObject root{
        {QStringLiteral("lnList"), QJsonArray{
            QJsonObject{
                {QStringLiteral("cjlx"), QStringLiteral("2025-2026学年秋")},
                {QStringLiteral("cjList"), QJsonArray{courseA, courseB}}
            }
        }}
    };
    const PassingScoreResult result = PassingScoreResult::fromJson(root);
    QCOMPARE(result.totalPassed(), 2);
    QCOMPARE(result.gpa(), 3.6);
    QCOMPARE(result.weightedAvgScore(), 86.0);
}

// ---------------------------------------------------------------------------
// Grade: Custom stats
// ---------------------------------------------------------------------------

void DModuleTest::customStatsForSelectedCourses()
{
    GradeCourseItem a;
    a.courseName = QStringLiteral("A");
    a.gradeName = QStringLiteral("A");
    a.credit = QStringLiteral("3");
    a.courseScore = 90.0;
    a.gradePointScore = 4.0;
    a.courseAttributeName = QStringLiteral("必修");
    a.passed = true;
    a.hasEffectiveScore = true;

    GradeCourseItem b;
    b.courseName = QStringLiteral("B");
    b.gradeName = QStringLiteral("B");
    b.credit = QStringLiteral("2");
    b.courseScore = 80.0;
    b.gradePointScore = 3.0;
    b.courseAttributeName = QStringLiteral("必修");
    b.passed = true;
    b.hasEffectiveScore = true;

    const QVariantMap stats = customStatsForCourses({a, b});
    QCOMPARE(stats.value(QStringLiteral("selectedCount")).toInt(), 2);
    QCOMPARE(stats.value(QStringLiteral("passedCount")).toInt(), 2);
    QCOMPARE(stats.value(QStringLiteral("gpa")).toDouble(), 3.6);
    QCOMPARE(stats.value(QStringLiteral("credits")).toDouble(), 5.0);
    QCOMPARE(stats.value(QStringLiteral("weightedAvgScore")).toDouble(), 86.0);
}

void DModuleTest::customStatsForEmptyList()
{
    const QVariantMap stats = customStatsForCourses({});
    QCOMPARE(stats.value(QStringLiteral("selectedCount")).toInt(), 0);
    QCOMPARE(stats.value(QStringLiteral("gpa")).toDouble(), 0.0);
    QCOMPARE(stats.value(QStringLiteral("credits")).toDouble(), 0.0);
}

// ---------------------------------------------------------------------------
// Grade: Term grouping
// ---------------------------------------------------------------------------

void DModuleTest::groupsByTermAndSorts()
{
    GradeCourseItem autumn;
    autumn.courseName = QStringLiteral("秋课");
    autumn.academicYearCode = QStringLiteral("2025-2026");
    autumn.termName = QStringLiteral("秋");
    autumn.gradeName = QStringLiteral("A");
    autumn.credit = QStringLiteral("3");
    autumn.courseScore = 90.0;
    autumn.gradePointScore = 4.0;
    autumn.passed = true;
    autumn.hasEffectiveScore = true;

    GradeCourseItem spring;
    spring.courseName = QStringLiteral("春课");
    spring.academicYearCode = QStringLiteral("2025-2026");
    spring.termName = QStringLiteral("春");
    spring.gradeName = QStringLiteral("A");
    spring.credit = QStringLiteral("2");
    spring.courseScore = 85.0;
    spring.gradePointScore = 3.7;
    spring.passed = true;
    spring.hasEffectiveScore = true;

    GradeCourseItem older;
    older.courseName = QStringLiteral("旧课");
    older.academicYearCode = QStringLiteral("2024-2025");
    older.termName = QStringLiteral("秋");
    older.gradeName = QStringLiteral("B");
    older.credit = QStringLiteral("2");
    older.courseScore = 80.0;
    older.gradePointScore = 3.0;
    older.passed = true;
    older.hasEffectiveScore = true;

    const QList<TermGradeGroup> groups = groupGradeCoursesByTerm({autumn, spring, older});
    QCOMPARE(groups.size(), 3);
    QCOMPARE(groups.first().label, QStringLiteral("2025-2026 春"));
    QCOMPARE(groups.at(1).label, QStringLiteral("2025-2026 秋"));
    QCOMPARE(groups.last().label, QStringLiteral("2024-2025 秋"));
}

// ---------------------------------------------------------------------------
// Grade: credits by attribute
// ---------------------------------------------------------------------------

void DModuleTest::creditsByAttribute()
{
    GradeCourseItem required;
    required.courseName = QStringLiteral("必修");
    required.courseAttributeName = QStringLiteral("必修");
    required.credit = QStringLiteral("3");
    required.gradeName = QStringLiteral("A");
    required.courseScore = 90.0;
    required.gradePointScore = 4.0;
    required.passed = true;
    required.hasEffectiveScore = true;

    GradeCourseItem elective;
    elective.courseName = QStringLiteral("选修");
    elective.courseAttributeName = QStringLiteral("选修");
    elective.credit = QStringLiteral("2");
    elective.gradeName = QStringLiteral("B");
    elective.courseScore = 80.0;
    elective.gradePointScore = 3.0;
    elective.passed = true;
    elective.hasEffectiveScore = true;

    GradeCourseItem optional;
    optional.courseName = QStringLiteral("任选");
    optional.courseAttributeName = QStringLiteral("任选");
    optional.credit = QStringLiteral("1");
    optional.gradeName = QStringLiteral("C");
    optional.courseScore = 70.0;
    optional.gradePointScore = 2.0;
    optional.passed = true;
    optional.hasEffectiveScore = true;

    const QJsonObject root{
        {QStringLiteral("lnList"), QJsonArray{
            QJsonObject{
                {QStringLiteral("cjList"), QJsonArray{
                    QJsonObject{
                        {QStringLiteral("courseName"), required.courseName},
                        {QStringLiteral("courseAttributeName"), required.courseAttributeName},
                        {QStringLiteral("credit"), required.credit},
                        {QStringLiteral("gradeName"), required.gradeName},
                        {QStringLiteral("courseScore"), required.courseScore},
                        {QStringLiteral("gradePointScore"), required.gradePointScore}
                    },
                    QJsonObject{
                        {QStringLiteral("courseName"), elective.courseName},
                        {QStringLiteral("courseAttributeName"), elective.courseAttributeName},
                        {QStringLiteral("credit"), elective.credit},
                        {QStringLiteral("gradeName"), elective.gradeName},
                        {QStringLiteral("courseScore"), elective.courseScore},
                        {QStringLiteral("gradePointScore"), elective.gradePointScore}
                    },
                    QJsonObject{
                        {QStringLiteral("courseName"), optional.courseName},
                        {QStringLiteral("courseAttributeName"), optional.courseAttributeName},
                        {QStringLiteral("credit"), optional.credit},
                        {QStringLiteral("gradeName"), optional.gradeName},
                        {QStringLiteral("courseScore"), optional.courseScore},
                        {QStringLiteral("gradePointScore"), optional.gradePointScore}
                    }
                }}
            }
        }}
    };
    const SchemeScoreSummary summary = SchemeScoreSummary::fromJson(root);
    QCOMPARE(summary.creditsByAttribute(QStringLiteral("必修")), 3.0);
    QCOMPARE(summary.creditsByAttribute(QStringLiteral("选修")), 2.0);
    QCOMPARE(summary.creditsByAttribute(QStringLiteral("任选")), 1.0);
}

// ---------------------------------------------------------------------------
// GradeStatisticsService wrappers
// ---------------------------------------------------------------------------

void DModuleTest::gradeStatisticsServiceWrapsParsing()
{
    GradeStatisticsService svc;

    const QJsonObject schemeRoot{
        {QStringLiteral("lnList"), QJsonArray{
            QJsonObject{
                {QStringLiteral("zxf"), 160.0},
                {QStringLiteral("yxxf"), 10.0},
                {QStringLiteral("zms"), 3},
                {QStringLiteral("tgms"), 3},
                {QStringLiteral("wtgms"), 0},
                {QStringLiteral("cjList"), QJsonArray{
                    QJsonObject{
                        {QStringLiteral("courseName"), QStringLiteral("Test")},
                        {QStringLiteral("gradeName"), QStringLiteral("A")},
                        {QStringLiteral("courseScore"), 90.0},
                        {QStringLiteral("gradePointScore"), 4.0},
                        {QStringLiteral("credit"), QStringLiteral("3")},
                        {QStringLiteral("courseAttributeName"), QStringLiteral("必修")}
                    }
                }}
            }
        }}
    };
    const SchemeScoreSummary schemeSummary = svc.parseSchemeScores(schemeRoot);
    QCOMPARE(schemeSummary.items.size(), 1);
    QCOMPARE(schemeSummary.totalCredits, 160.0);

    const QJsonObject passingRoot{
        {QStringLiteral("lnList"), QJsonArray{
            QJsonObject{
                {QStringLiteral("cjlx"), QStringLiteral("2025-2026学年春")},
                {QStringLiteral("cjList"), QJsonArray{}}
            }
        }}
    };
    const PassingScoreResult passingResult = svc.parsePassingScores(passingRoot);
    QCOMPARE(passingResult.groups.size(), 1);
    QCOMPARE(passingResult.groups.first().label, QStringLiteral("2025-2026学年春"));

    GradeCourseItem item;
    item.courseName = QStringLiteral("X");
    item.gradeName = QStringLiteral("A");
    item.credit = QStringLiteral("4");
    item.courseScore = 88.0;
    item.gradePointScore = 3.8;
    item.courseAttributeName = QStringLiteral("必修");
    item.passed = true;
    item.hasEffectiveScore = true;

    const QVariantMap custom = svc.customStats({item});
    QCOMPARE(custom.value(QStringLiteral("selectedCount")).toInt(), 1);
    QCOMPARE(custom.value(QStringLiteral("gpa")).toDouble(), 3.8);
}

QTEST_MAIN(DModuleTest)
#include "tst_d_module.moc"
