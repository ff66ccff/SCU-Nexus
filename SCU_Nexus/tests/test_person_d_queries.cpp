#include "src/common/QueryState.h"
#include "src/models/ExamPlanModels.h"
#include "src/models/GradeModels.h"
#include "src/repositories/QueryCacheRepository.h"
#include "src/services/calendar/AcademicCalendarService.h"
#include "src/services/grades/GradeStatisticsService.h"
#include "src/services/zhjw/ZhjwQueryService.h"
#include "src/viewmodels/ExamPlanViewModel.h"
#include "src/viewmodels/GradesViewModel.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>
#include <utility>

class FakeZhjwQueryService : public ZhjwQueryService
{
public:
    bool loggedInValue = false;
    QList<ExamPlanItemDto> examItems;
    QJsonObject schemeScores;
    QJsonObject passingScores;
    ApiError nextError;

    bool loggedIn() const override { return loggedInValue; }
    void setLoggedIn(bool loggedIn) override
    {
        if (loggedInValue == loggedIn) {
            return;
        }
        loggedInValue = loggedIn;
        emit loggedInChanged();
    }

    void fetchExamPlan(ExamPlanCallback callback) override
    {
        callback(examItems, std::exchange(nextError, ApiError{}));
    }

    void fetchSchemeScores(JsonCallback callback) override
    {
        callback(schemeScores, std::exchange(nextError, ApiError{}));
    }

    void fetchPassingScores(JsonCallback callback) override
    {
        callback(passingScores, std::exchange(nextError, ApiError{}));
    }
};

class DeferredZhjwQueryService : public ZhjwQueryService
{
public:
    bool loggedInValue = true;
    int examFetchCount = 0;
    int schemeFetchCount = 0;
    int passingFetchCount = 0;
    QList<ExamPlanCallback> pendingExamCallbacks;
    QList<JsonCallback> pendingSchemeCallbacks;
    QList<JsonCallback> pendingPassingCallbacks;

    bool loggedIn() const override { return loggedInValue; }
    void setLoggedIn(bool loggedIn) override
    {
        if (loggedInValue == loggedIn) {
            return;
        }
        loggedInValue = loggedIn;
        emit loggedInChanged();
    }

    void fetchExamPlan(ExamPlanCallback callback) override
    {
        ++examFetchCount;
        pendingExamCallbacks.append(std::move(callback));
    }

    void fetchSchemeScores(JsonCallback callback) override
    {
        ++schemeFetchCount;
        pendingSchemeCallbacks.append(std::move(callback));
    }

    void fetchPassingScores(JsonCallback callback) override
    {
        ++passingFetchCount;
        pendingPassingCallbacks.append(std::move(callback));
    }

    void completeExam(const QList<ExamPlanItemDto> &items, const ApiError &error = {})
    {
        QVERIFY(!pendingExamCallbacks.isEmpty());
        auto callback = std::move(pendingExamCallbacks.first());
        pendingExamCallbacks.removeFirst();
        callback(items, error);
    }

    void completeScheme(const QJsonObject &root, const ApiError &error = {})
    {
        QVERIFY(!pendingSchemeCallbacks.isEmpty());
        auto callback = std::move(pendingSchemeCallbacks.first());
        pendingSchemeCallbacks.removeFirst();
        callback(root, error);
    }

    void completePassing(const QJsonObject &root, const ApiError &error = {})
    {
        QVERIFY(!pendingPassingCallbacks.isEmpty());
        auto callback = std::move(pendingPassingCallbacks.first());
        pendingPassingCallbacks.removeFirst();
        callback(root, error);
    }
};

class PersonDQueryTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesCalendarEntriesAndImages();
    void sortsExamItemsByStartTimeWithUnparsedAtEnd();
    void calculatesSchemeAndCustomGradeStats();
    void parsesNumericGradeScalarsAsText();
    void parsesBooleanGradeScalarsAsText();
    void doesNotStringifyStructuredGradeValues();
    void fallsBackRawScoreToCourseScoreWithoutReplacingGradeName();
    void preservesAllFailedServerCounters();
    void computesOnlyMissingSchemeCounters();
    void defaultsMissingNumericScoresToZero();
    void excludesIneffectiveCoursesFromCreditStats();
    void ignoresDuplicateExamRefreshWhileLoading();
    void ignoresDuplicateGradeRefreshesWhileLoading();
    void examViewModelUsesCacheWhenLoggedOut();
    void gradesViewModelKeepsCacheWhenRefreshFails();
};

void PersonDQueryTest::parsesCalendarEntriesAndImages()
{
    const QString html = QStringLiteral(R"(
        <a href="info/1101/1234.htm">四川大学 2025-2026 学年校历</a>
        <a href="info/1101/1235.htm">四川大学 2024-2025 学年校历</a>
        <img src="/__local/AB/CD/calendar.jpg">
    )");

    const QList<AcademicCalendarEntry> entries = AcademicCalendarService::parseEntries(html);
    QCOMPARE(entries.size(), 2);
    QCOMPARE(entries.first().title, QStringLiteral("2025-2026"));
    QCOMPARE(entries.first().path, QStringLiteral("info/1101/1234.htm"));

    const QStringList imageUrls = AcademicCalendarService::parseImageUrls(html);
    QCOMPARE(imageUrls.size(), 1);
    QCOMPARE(imageUrls.first(), QStringLiteral("https://jwc.scu.edu.cn/__local/AB/CD/calendar.jpg"));
}

void PersonDQueryTest::sortsExamItemsByStartTimeWithUnparsedAtEnd()
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

    ExamPlanItem unparsed;
    unparsed.courseName = QStringLiteral("C");
    unparsed.date = QStringLiteral("待定");
    updateExamTemporalFields(&unparsed, QDateTime(QDate(2026, 1, 1), QTime(8, 0)));

    QList<ExamPlanItem> items{unparsed, late, early};
    sortExamPlanItems(&items);

    QCOMPARE(items.at(0).courseName, QStringLiteral("A"));
    QCOMPARE(items.at(1).courseName, QStringLiteral("B"));
    QCOMPARE(items.at(2).courseName, QStringLiteral("C"));
}

void PersonDQueryTest::calculatesSchemeAndCustomGradeStats()
{
    const QJsonObject root{
        {QStringLiteral("lnList"), QJsonArray{
            QJsonObject{
                {QStringLiteral("zxf"), 160.0},
                {QStringLiteral("yxxf"), 8.0},
                {QStringLiteral("tgms"), 2},
                {QStringLiteral("wtgms"), 0},
                {QStringLiteral("zms"), 2},
                {QStringLiteral("cjList"), QJsonArray{
                    QJsonObject{
                        {QStringLiteral("courseName"), QStringLiteral("A")},
                        {QStringLiteral("courseAttributeName"), QStringLiteral("必修")},
                        {QStringLiteral("credit"), QStringLiteral("3")},
                        {QStringLiteral("courseScore"), 90.0},
                        {QStringLiteral("gradePointScore"), 4.0},
                        {QStringLiteral("gradeName"), QStringLiteral("A")},
                        {QStringLiteral("academicYearCode"), QStringLiteral("2025-2026")},
                        {QStringLiteral("termName"), QStringLiteral("秋")}
                    },
                    QJsonObject{
                        {QStringLiteral("courseName"), QStringLiteral("B")},
                        {QStringLiteral("courseAttributeName"), QStringLiteral("选修")},
                        {QStringLiteral("credit"), QStringLiteral("2")},
                        {QStringLiteral("courseScore"), 80.0},
                        {QStringLiteral("gradePointScore"), 3.0},
                        {QStringLiteral("gradeName"), QStringLiteral("B")},
                        {QStringLiteral("academicYearCode"), QStringLiteral("2025-2026")},
                        {QStringLiteral("termName"), QStringLiteral("春")}
                    }
                }}
            }
        }}
    };

    const SchemeScoreSummary summary = SchemeScoreSummary::fromJson(root);
    QCOMPARE(summary.items.size(), 2);
    QCOMPARE(summary.gpa(), 3.6);
    QCOMPARE(summary.requiredGpa(), 4.0);
    QCOMPARE(summary.weightedAvgScore(), 86.0);

    GradeStatisticsService service;
    const QVariantMap custom = service.customStats(summary.items);
    QCOMPARE(custom.value(QStringLiteral("selectedCount")).toInt(), 2);
    QCOMPARE(custom.value(QStringLiteral("credits")).toDouble(), 5.0);
    QCOMPARE(custom.value(QStringLiteral("weightedAvgScore")).toDouble(), 86.0);
}

void PersonDQueryTest::parsesNumericGradeScalarsAsText()
{
    const GradeCourseItem item = GradeCourseItem::fromJson(QJsonObject{
        {QStringLiteral("courseName"), 101},
        {QStringLiteral("englishCourseName"), 202},
        {QStringLiteral("courseAttributeName"), 303},
        {QStringLiteral("credit"), 3.5},
        {QStringLiteral("cj"), 87},
        {QStringLiteral("courseScore"), 91.25},
        {QStringLiteral("gradePointScore"), 3.7},
        {QStringLiteral("gradeName"), 4},
        {QStringLiteral("academicYearCode"), 123456789012345.0},
        {QStringLiteral("termName"), 2}
    });

    QCOMPARE(item.courseName, QStringLiteral("101"));
    QCOMPARE(item.englishCourseName, QStringLiteral("202"));
    QCOMPARE(item.courseAttributeName, QStringLiteral("303"));
    QCOMPARE(item.credit, QStringLiteral("3.5"));
    QCOMPARE(item.rawScore, QStringLiteral("87"));
    QCOMPARE(item.gradeName, QStringLiteral("4"));
    QCOMPARE(item.academicYearCode, QStringLiteral("123456789012345"));
    QCOMPARE(item.termName, QStringLiteral("2"));

    const SchemeScoreSummary scheme = SchemeScoreSummary::fromJson(QJsonObject{
        {QStringLiteral("lnList"), QJsonArray{QJsonObject{{QStringLiteral("cjlx"), 12}}}}
    });
    QCOMPARE(scheme.scoreType, QStringLiteral("12"));

    const PassingScoreResult passing = PassingScoreResult::fromJson(QJsonObject{
        {QStringLiteral("lnList"), QJsonArray{QJsonObject{{QStringLiteral("cjlx"), 2026}}}}
    });
    QCOMPARE(passing.groups.size(), 1);
    QCOMPARE(passing.groups.first().label, QStringLiteral("2026"));
}

void PersonDQueryTest::parsesBooleanGradeScalarsAsText()
{
    const GradeCourseItem item = GradeCourseItem::fromJson(QJsonObject{
        {QStringLiteral("courseName"), true},
        {QStringLiteral("englishCourseName"), false},
        {QStringLiteral("courseAttributeName"), true},
        {QStringLiteral("credit"), false},
        {QStringLiteral("cj"), true},
        {QStringLiteral("gradeName"), false},
        {QStringLiteral("academicYearCode"), true},
        {QStringLiteral("termName"), false}
    });

    QCOMPARE(item.courseName, QStringLiteral("true"));
    QCOMPARE(item.englishCourseName, QStringLiteral("false"));
    QCOMPARE(item.courseAttributeName, QStringLiteral("true"));
    QCOMPARE(item.credit, QStringLiteral("false"));
    QCOMPARE(item.rawScore, QStringLiteral("true"));
    QCOMPARE(item.gradeName, QStringLiteral("false"));
    QCOMPARE(item.academicYearCode, QStringLiteral("true"));
    QCOMPARE(item.termName, QStringLiteral("false"));

    const SchemeScoreSummary scheme = SchemeScoreSummary::fromJson(QJsonObject{
        {QStringLiteral("lnList"), QJsonArray{QJsonObject{{QStringLiteral("cjlx"), true}}}}
    });
    QCOMPARE(scheme.scoreType, QStringLiteral("true"));

    const PassingScoreResult passing = PassingScoreResult::fromJson(QJsonObject{
        {QStringLiteral("lnList"), QJsonArray{QJsonObject{{QStringLiteral("cjlx"), false}}}}
    });
    QCOMPARE(passing.groups.size(), 1);
    QCOMPARE(passing.groups.first().label, QStringLiteral("false"));
}

void PersonDQueryTest::doesNotStringifyStructuredGradeValues()
{
    const GradeCourseItem item = GradeCourseItem::fromJson(QJsonObject{
        {QStringLiteral("courseName"), QJsonObject{{QStringLiteral("value"), 1}}},
        {QStringLiteral("englishCourseName"), QJsonArray{QStringLiteral("English")}},
        {QStringLiteral("courseAttributeName"), QJsonValue(QJsonValue::Null)},
        {QStringLiteral("credit"), QJsonObject{{QStringLiteral("value"), 3}}},
        {QStringLiteral("cj"), QJsonArray{90}},
        {QStringLiteral("gradeName"), QJsonValue(QJsonValue::Null)},
        {QStringLiteral("academicYearCode"), QJsonObject{{QStringLiteral("value"), 2026}}},
        {QStringLiteral("termName"), QJsonArray{QStringLiteral("春")}}
    });

    QVERIFY(item.courseName.isEmpty());
    QVERIFY(item.englishCourseName.isEmpty());
    QVERIFY(item.courseAttributeName.isEmpty());
    QVERIFY(item.credit.isEmpty());
    QVERIFY(item.rawScore.isEmpty());
    QVERIFY(item.gradeName.isEmpty());
    QVERIFY(item.academicYearCode.isEmpty());
    QVERIFY(item.termName.isEmpty());

    const SchemeScoreSummary scheme = SchemeScoreSummary::fromJson(QJsonObject{
        {QStringLiteral("lnList"), QJsonArray{QJsonObject{
            {QStringLiteral("cjlx"), QJsonObject{{QStringLiteral("value"), 1}}}
        }}}
    });
    QVERIFY(scheme.scoreType.isEmpty());

    const PassingScoreResult passing = PassingScoreResult::fromJson(QJsonObject{
        {QStringLiteral("lnList"), QJsonArray{QJsonObject{
            {QStringLiteral("cjlx"), QJsonArray{QStringLiteral("2025-2026")}}
        }}}
    });
    QCOMPARE(passing.groups.size(), 1);
    QVERIFY(passing.groups.first().label.isEmpty());
}

void PersonDQueryTest::fallsBackRawScoreToCourseScoreWithoutReplacingGradeName()
{
    const GradeCourseItem scoreFallback = GradeCourseItem::fromJson(QJsonObject{
        {QStringLiteral("courseName"), QStringLiteral("编译原理")},
        {QStringLiteral("courseScore"), 92.5},
        {QStringLiteral("gradePointScore"), 4.0},
        {QStringLiteral("gradeName"), QStringLiteral("A")}
    });
    QCOMPARE(scoreFallback.rawScore, QStringLiteral("92.5"));
    QCOMPARE(scoreFallback.gradeName, QStringLiteral("A"));

    const GradeCourseItem gradeOnly = GradeCourseItem::fromJson(QJsonObject{
        {QStringLiteral("courseName"), QStringLiteral("离散数学")},
        {QStringLiteral("gradeName"), QStringLiteral("B+")}
    });
    QVERIFY(gradeOnly.rawScore.isEmpty());
    QCOMPARE(gradeOnly.gradeName, QStringLiteral("B+"));
}

void PersonDQueryTest::preservesAllFailedServerCounters()
{
    const QJsonObject root{
        {QStringLiteral("lnList"), QJsonArray{QJsonObject{
            {QStringLiteral("tgms"), 0},
            {QStringLiteral("wtgms"), 2},
            {QStringLiteral("zms"), 2},
            {QStringLiteral("cjList"), QJsonArray{
                QJsonObject{
                    {QStringLiteral("courseName"), QStringLiteral("A")},
                    {QStringLiteral("credit"), QStringLiteral("2")},
                    {QStringLiteral("gradeName"), QStringLiteral("F")}
                },
                QJsonObject{
                    {QStringLiteral("courseName"), QStringLiteral("B")},
                    {QStringLiteral("credit"), QStringLiteral("2")},
                    {QStringLiteral("gradeName"), QStringLiteral("F")}
                }
            }}
        }}}
    };

    const SchemeScoreSummary summary = SchemeScoreSummary::fromJson(root);
    QCOMPARE(summary.passedCount, 0);
    QCOMPARE(summary.failedCount, 2);
    QCOMPARE(summary.totalCount, 2);
}

void PersonDQueryTest::computesOnlyMissingSchemeCounters()
{
    const QJsonArray courses{
        QJsonObject{
            {QStringLiteral("courseName"), QStringLiteral("A")},
            {QStringLiteral("credit"), QStringLiteral("2")},
            {QStringLiteral("gradeName"), QStringLiteral("A")}
        },
        QJsonObject{
            {QStringLiteral("courseName"), QStringLiteral("B")},
            {QStringLiteral("credit"), QStringLiteral("2")},
            {QStringLiteral("gradeName"), QStringLiteral("F")}
        }
    };

    const SchemeScoreSummary missingBoth = SchemeScoreSummary::fromJson(QJsonObject{
        {QStringLiteral("lnList"), QJsonArray{QJsonObject{
            {QStringLiteral("zms"), 0},
            {QStringLiteral("cjList"), courses}
        }}}
    });
    QCOMPARE(missingBoth.passedCount, 1);
    QCOMPARE(missingBoth.failedCount, 1);
    QCOMPARE(missingBoth.totalCount, 2);

    const SchemeScoreSummary failedProvided = SchemeScoreSummary::fromJson(QJsonObject{
        {QStringLiteral("lnList"), QJsonArray{QJsonObject{
            {QStringLiteral("wtgms"), 0},
            {QStringLiteral("cjList"), courses}
        }}}
    });
    QCOMPARE(failedProvided.passedCount, 1);
    QCOMPARE(failedProvided.failedCount, 0);
    QCOMPARE(failedProvided.totalCount, 2);

    const SchemeScoreSummary passedProvided = SchemeScoreSummary::fromJson(QJsonObject{
        {QStringLiteral("lnList"), QJsonArray{QJsonObject{
            {QStringLiteral("tgms"), 0},
            {QStringLiteral("zms"), 2},
            {QStringLiteral("cjList"), courses}
        }}}
    });
    QCOMPARE(passedProvided.passedCount, 0);
    QCOMPARE(passedProvided.failedCount, 1);
    QCOMPARE(passedProvided.totalCount, 2);
}

void PersonDQueryTest::defaultsMissingNumericScoresToZero()
{
    const GradeCourseItem item = GradeCourseItem::fromJson(QJsonObject{
        {QStringLiteral("courseName"), QStringLiteral("缺省数值课程")},
        {QStringLiteral("courseAttributeName"), QStringLiteral("必修")},
        {QStringLiteral("credit"), QStringLiteral("2")},
        {QStringLiteral("gradeName"), QStringLiteral("B")}
    });

    QCOMPARE(item.courseScore, 0.0);
    QCOMPARE(item.gradePointScore, 0.0);
    QVERIFY(item.hasEffectiveScore);

    const QVariantMap custom = customStatsForCourses(QList<GradeCourseItem>{item});
    QCOMPARE(custom.value(QStringLiteral("credits")).toDouble(), 2.0);
    QCOMPARE(custom.value(QStringLiteral("passedCount")).toInt(), 1);
    QCOMPARE(custom.value(QStringLiteral("weightedAvgScore")).toDouble(), 0.0);
}

void PersonDQueryTest::excludesIneffectiveCoursesFromCreditStats()
{
    const QJsonObject root{
        {QStringLiteral("lnList"), QJsonArray{
            QJsonObject{
                {QStringLiteral("zxf"), 160.0},
                {QStringLiteral("yxxf"), 5.0},
                {QStringLiteral("tgms"), 3},
                {QStringLiteral("wtgms"), 1},
                {QStringLiteral("zms"), 4},
                {QStringLiteral("cjList"), QJsonArray{
                    QJsonObject{
                        {QStringLiteral("courseName"), QStringLiteral("有效必修")},
                        {QStringLiteral("courseAttributeName"), QStringLiteral("必修")},
                        {QStringLiteral("credit"), QStringLiteral("3")},
                        {QStringLiteral("courseScore"), 90.0},
                        {QStringLiteral("gradePointScore"), 4.0},
                        {QStringLiteral("gradeName"), QStringLiteral("A")},
                        {QStringLiteral("academicYearCode"), QStringLiteral("2025-2026")},
                        {QStringLiteral("termName"), QStringLiteral("秋")}
                    },
                    QJsonObject{
                        {QStringLiteral("courseName"), QStringLiteral("无效选修")},
                        {QStringLiteral("courseAttributeName"), QStringLiteral("选修")},
                        {QStringLiteral("credit"), QStringLiteral("2")},
                        {QStringLiteral("courseScore"), -1.0},
                        {QStringLiteral("gradePointScore"), -1.0},
                        {QStringLiteral("gradeName"), QStringLiteral("B")},
                        {QStringLiteral("academicYearCode"), QStringLiteral("2025-2026")},
                        {QStringLiteral("termName"), QStringLiteral("秋")}
                    },
                    QJsonObject{
                        {QStringLiteral("courseName"), QStringLiteral("无效未通过")},
                        {QStringLiteral("courseAttributeName"), QStringLiteral("任选")},
                        {QStringLiteral("credit"), QStringLiteral("1")},
                        {QStringLiteral("courseScore"), -1.0},
                        {QStringLiteral("gradePointScore"), -1.0},
                        {QStringLiteral("gradeName"), QStringLiteral("F")},
                        {QStringLiteral("academicYearCode"), QStringLiteral("2025-2026")},
                        {QStringLiteral("termName"), QStringLiteral("秋")}
                    }
                }}
            }
        }}
    };

    const SchemeScoreSummary summary = SchemeScoreSummary::fromJson(root);
    QCOMPARE(summary.earnedCredits(), 3.0);
    QCOMPARE(summary.creditsByAttribute(QStringLiteral("必修")), 3.0);
    QCOMPARE(summary.creditsByAttribute(QStringLiteral("选修")), 0.0);
    QCOMPARE(summary.creditsByAttribute(QStringLiteral("任选")), 0.0);

    GradeStatisticsService service;
    const QVariantMap custom = service.customStats(summary.items);
    QCOMPARE(custom.value(QStringLiteral("credits")).toDouble(), 3.0);
    QCOMPARE(custom.value(QStringLiteral("passedCount")).toInt(), 1);
    QCOMPARE(custom.value(QStringLiteral("failedCount")).toInt(), 0);
    QCOMPARE(custom.value(QStringLiteral("selectedCount")).toInt(), 3);

    const QJsonObject passingRoot{
        {QStringLiteral("lnList"), QJsonArray{
            QJsonObject{
                {QStringLiteral("cjlx"), QStringLiteral("2025-2026学年秋(两学期)")},
                {QStringLiteral("cjList"), root.value(QStringLiteral("lnList")).toArray().first().toObject().value(QStringLiteral("cjList")).toArray()}
            }
        }}
    };
    const PassingScoreResult passing = PassingScoreResult::fromJson(passingRoot);
    QCOMPARE(passing.totalCredits(), 3.0);
    QCOMPARE(passing.totalPassed(), 1);
    QCOMPARE(passing.groups.first().toVariant().value(QStringLiteral("credits")).toDouble(), 3.0);
    QCOMPARE(passing.groups.first().toVariant().value(QStringLiteral("passedCount")).toInt(), 1);
}

void PersonDQueryTest::ignoresDuplicateExamRefreshWhileLoading()
{
    DeferredZhjwQueryService service;
    ExamPlanViewModel model(nullptr, &service);

    model.refresh();
    model.refresh();

    QCOMPARE(model.state(), QueryState::Loading);
    QCOMPARE(service.examFetchCount, 1);
    service.completeExam(QList<ExamPlanItemDto>{
        ExamPlanItemDto{
            QStringLiteral("编译原理"),
            QStringLiteral("第18周"),
            QStringLiteral("2026-01-08"),
            QStringLiteral("星期四"),
            QStringLiteral("09:00-11:00"),
            QStringLiteral("江安一教 A101"),
            QStringLiteral("12"),
            QStringLiteral("SCU20260108001"),
            QStringLiteral("无")
        }
    });
    QCOMPARE(model.state(), QueryState::Loaded);
    QCOMPARE(model.count(), 1);
}

void PersonDQueryTest::ignoresDuplicateGradeRefreshesWhileLoading()
{
    DeferredZhjwQueryService service;
    GradesViewModel model(nullptr, &service);

    model.refreshSchemeScores();
    model.refreshSchemeScores();
    QCOMPARE(model.schemeState(), QueryState::Loading);
    QCOMPARE(service.schemeFetchCount, 1);

    model.refreshPassingScores();
    model.refreshPassingScores();
    QCOMPARE(model.passingState(), QueryState::Loading);
    QCOMPARE(service.passingFetchCount, 1);

    service.completeScheme(QJsonObject{{QStringLiteral("lnList"), QJsonArray{}}});
    service.completePassing(QJsonObject{{QStringLiteral("lnList"), QJsonArray{}}});
    QCOMPARE(model.schemeState(), QueryState::Empty);
    QCOMPARE(model.passingState(), QueryState::Empty);
}

void PersonDQueryTest::examViewModelUsesCacheWhenLoggedOut()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));
    QVERIFY(cache.open());

    FakeZhjwQueryService service;
    service.loggedInValue = true;
    service.examItems = {
        ExamPlanItemDto{
            QStringLiteral("数据结构"),
            QStringLiteral("第18周"),
            QStringLiteral("2026-01-08"),
            QStringLiteral("星期四"),
            QStringLiteral("09:00-11:00"),
            QStringLiteral("江安一教 A101"),
            QStringLiteral("12"),
            QStringLiteral("SCU20260108001"),
            QStringLiteral("无")
        }
    };

    ExamPlanViewModel writer(&cache, &service);
    writer.load();
    QCOMPARE(writer.state(), QueryState::Loaded);
    QCOMPARE(writer.count(), 1);

    service.loggedInValue = false;
    ExamPlanViewModel reader(&cache, &service);
    reader.load();
    QCOMPARE(reader.state(), QueryState::Loaded);
    QCOMPARE(reader.count(), 1);
    QCOMPARE(reader.exams().first().toMap().value(QStringLiteral("courseName")).toString(), QStringLiteral("数据结构"));
}

void PersonDQueryTest::gradesViewModelKeepsCacheWhenRefreshFails()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));
    QVERIFY(cache.open());

    const QJsonObject schemeRoot{
        {QStringLiteral("lnList"), QJsonArray{
            QJsonObject{
                {QStringLiteral("zxf"), 160.0},
                {QStringLiteral("yxxf"), 3.0},
                {QStringLiteral("tgms"), 1},
                {QStringLiteral("wtgms"), 0},
                {QStringLiteral("zms"), 1},
                {QStringLiteral("cjList"), QJsonArray{
                    QJsonObject{
                        {QStringLiteral("courseName"), QStringLiteral("高等数学")},
                        {QStringLiteral("courseAttributeName"), QStringLiteral("必修")},
                        {QStringLiteral("credit"), QStringLiteral("3")},
                        {QStringLiteral("courseScore"), 90.0},
                        {QStringLiteral("gradePointScore"), 4.0},
                        {QStringLiteral("gradeName"), QStringLiteral("A")}
                    }
                }}
            }
        }}
    };

    FakeZhjwQueryService service;
    service.loggedInValue = true;
    service.schemeScores = schemeRoot;
    service.passingScores = QJsonObject{{QStringLiteral("lnList"), QJsonArray{}}};

    GradesViewModel writer(&cache, &service);
    writer.load();
    QCOMPARE(writer.schemeState(), QueryState::Loaded);
    QCOMPARE(writer.schemeSummary().value(QStringLiteral("gpa")).toDouble(), 4.0);

    service.nextError = ApiError{ApiErrorType::Network, QStringLiteral("network down"), 503};
    writer.refreshSchemeScores();
    QCOMPARE(writer.schemeState(), QueryState::Loaded);
    QCOMPARE(writer.schemeErrorMessage(), QStringLiteral("network down"));
    QCOMPARE(writer.schemeSummary().value(QStringLiteral("gpa")).toDouble(), 4.0);
}

QTEST_MAIN(PersonDQueryTest)
#include "test_person_d_queries.moc"
