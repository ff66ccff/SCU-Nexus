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
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>
#include <QUuid>
#include <tuple>
#include <utility>

namespace {
constexpr auto ExamPlanCacheKey = "exam_plan.latest";
constexpr auto SchemeCacheKey = "grades.scheme_scores";
constexpr auto PassingCacheKey = "grades.passing_scores";

bool executeCacheSql(const QString &databasePath, const QString &sql)
{
    const QString connectionName = QStringLiteral("query-cache-test-%1")
                                       .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    bool ok = false;
    {
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        database.setDatabaseName(databasePath);
        if (database.open()) {
            QSqlQuery query(database);
            ok = query.exec(sql);
        }
        database.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
    return ok;
}

bool dropCacheTable(const QString &databasePath)
{
    return executeCacheSql(databasePath, QStringLiteral("DROP TABLE query_cache"));
}

bool createCacheTable(const QString &databasePath)
{
    return executeCacheSql(databasePath, QStringLiteral(
        "CREATE TABLE query_cache ("
        "cache_key TEXT PRIMARY KEY,"
        "payload_json TEXT NOT NULL,"
        "updated_at TEXT NOT NULL"
        ")"));
}

bool blockCacheDelete(const QString &databasePath, const QString &cacheKey)
{
    return executeCacheSql(databasePath, QStringLiteral(
        "CREATE TRIGGER block_cache_delete BEFORE DELETE ON query_cache "
        "WHEN OLD.cache_key = '%1' "
        "BEGIN SELECT RAISE(ABORT, 'blocked cache delete'); END").arg(cacheKey));
}

ExamPlanItemDto sampleExamDto()
{
    return ExamPlanItemDto{
        QStringLiteral("数据结构"),
        QStringLiteral("第18周"),
        QStringLiteral("2026-01-08"),
        QStringLiteral("星期四"),
        QStringLiteral("09:00-11:00"),
        QStringLiteral("江安一教 A101"),
        QStringLiteral("12"),
        QStringLiteral("SCU20260108001"),
        QStringLiteral("无")
    };
}

QJsonObject sampleSchemeRoot()
{
    return QJsonObject{
        {QStringLiteral("lnList"), QJsonArray{QJsonObject{
            {QStringLiteral("zxf"), 160.0},
            {QStringLiteral("yxxf"), 3.0},
            {QStringLiteral("tgms"), 1},
            {QStringLiteral("wtgms"), 0},
            {QStringLiteral("zms"), 1},
            {QStringLiteral("cjList"), QJsonArray{QJsonObject{
                {QStringLiteral("courseName"), QStringLiteral("高等数学")},
                {QStringLiteral("courseAttributeName"), QStringLiteral("必修")},
                {QStringLiteral("credit"), QStringLiteral("3")},
                {QStringLiteral("courseScore"), 90.0},
                {QStringLiteral("gradePointScore"), 4.0},
                {QStringLiteral("gradeName"), QStringLiteral("A")},
                {QStringLiteral("academicYearCode"), QStringLiteral("2025-2026")},
                {QStringLiteral("termName"), QStringLiteral("秋")}
            }}}
        }}}
    };
}

QJsonObject samplePassingRoot()
{
    return QJsonObject{
        {QStringLiteral("lnList"), QJsonArray{QJsonObject{
            {QStringLiteral("cjlx"), QStringLiteral("2025-2026学年秋")},
            {QStringLiteral("cjList"), sampleSchemeRoot()
                .value(QStringLiteral("lnList")).toArray().first().toObject()
                .value(QStringLiteral("cjList")).toArray()}
        }}}
    };
}
}

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

class SynchronousFailureZhjwQueryService : public ZhjwQueryService
{
public:
    int schemeFetchCount = 0;
    int passingFetchCount = 0;

    bool loggedIn() const override { return true; }
    void setLoggedIn(bool) override {}
    void fetchExamPlan(ExamPlanCallback callback) override
    {
        callback({}, ApiError{ApiErrorType::Network, QStringLiteral("同步考试请求失败"), 503});
    }
    void fetchSchemeScores(JsonCallback callback) override
    {
        ++schemeFetchCount;
        callback({}, ApiError{ApiErrorType::Network, QStringLiteral("同步方案请求失败"), 503});
    }
    void fetchPassingScores(JsonCallback callback) override
    {
        ++passingFetchCount;
        callback({}, ApiError{ApiErrorType::Network, QStringLiteral("同步及格请求失败"), 503});
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
    void repositoryOpenClearsPreviousError();
    void repositoryPutClearsPreviousError();
    void repositoryGetMissClearsPreviousError();
    void repositoryRemoveClearsPreviousError();
    void repositoryClearClearsPreviousError();
    void repositoryFailuresKeepCurrentError();
    void rejectsInvalidExamCaches_data();
    void rejectsInvalidExamCaches();
    void rejectsInvalidGradeCaches_data();
    void rejectsInvalidGradeCaches();
    void clearExamCacheResetsChangedStateOnce();
    void clearGradesCacheResetsChangedStateOnce();
    void examCacheRemovalFailureIsVisible();
    void gradesCacheRemovalFailuresAreVisible();
    void invalidExamCacheRemovalFailureIsSafe();
    void invalidGradeCacheRemovalFailuresAreSafe_data();
    void invalidGradeCacheRemovalFailuresAreSafe();
    void partialGradeCacheRemovalFailureIsVisible_data();
    void partialGradeCacheRemovalFailureIsVisible();
    void offlineExamCacheReadFailureClearsInMemoryCache();
    void offlineGradeCacheReadFailuresClearInMemoryCaches();
    void onlineGradeCacheReadFailuresTriggerRefresh();
    void onlineGradeCacheReadFailureWithoutMemoryFetchesOnce();
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

void PersonDQueryTest::repositoryOpenClearsPreviousError()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));

    QueryCacheEntry entry;
    QVERIFY(!cache.get(QStringLiteral("before-open"), &entry));
    QVERIFY(!cache.lastError().isEmpty());

    QVERIFY2(cache.open(), qPrintable(cache.lastError()));
    QVERIFY(cache.lastError().isEmpty());
}

void PersonDQueryTest::repositoryPutClearsPreviousError()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString databasePath = dir.filePath(QStringLiteral("query-cache.sqlite"));
    QueryCacheRepository cache(databasePath);
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));

    QVERIFY(dropCacheTable(databasePath));
    QueryCacheEntry entry;
    QVERIFY(!cache.get(QStringLiteral("broken"), &entry));
    QVERIFY(!cache.lastError().isEmpty());
    QVERIFY(createCacheTable(databasePath));

    QVERIFY2(cache.put(QStringLiteral("fresh"), QStringLiteral("[]")), qPrintable(cache.lastError()));
    QVERIFY(cache.lastError().isEmpty());
}

void PersonDQueryTest::repositoryGetMissClearsPreviousError()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString databasePath = dir.filePath(QStringLiteral("query-cache.sqlite"));
    QueryCacheRepository cache(databasePath);
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));

    QVERIFY(dropCacheTable(databasePath));
    QueryCacheEntry entry;
    QVERIFY(!cache.get(QStringLiteral("broken"), &entry));
    QVERIFY(!cache.lastError().isEmpty());
    QVERIFY(createCacheTable(databasePath));

    QVERIFY(!cache.get(QStringLiteral("missing"), &entry));
    QVERIFY(cache.lastError().isEmpty());
}

void PersonDQueryTest::repositoryRemoveClearsPreviousError()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString databasePath = dir.filePath(QStringLiteral("query-cache.sqlite"));
    QueryCacheRepository cache(databasePath);
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));

    QVERIFY(dropCacheTable(databasePath));
    QueryCacheEntry entry;
    QVERIFY(!cache.get(QStringLiteral("broken"), &entry));
    QVERIFY(!cache.lastError().isEmpty());
    QVERIFY(createCacheTable(databasePath));

    QVERIFY2(cache.remove(QStringLiteral("missing")), qPrintable(cache.lastError()));
    QVERIFY(cache.lastError().isEmpty());
}

void PersonDQueryTest::repositoryClearClearsPreviousError()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString databasePath = dir.filePath(QStringLiteral("query-cache.sqlite"));
    QueryCacheRepository cache(databasePath);
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));

    QVERIFY(dropCacheTable(databasePath));
    QueryCacheEntry entry;
    QVERIFY(!cache.get(QStringLiteral("broken"), &entry));
    QVERIFY(!cache.lastError().isEmpty());
    QVERIFY(createCacheTable(databasePath));

    QVERIFY2(cache.clear(), qPrintable(cache.lastError()));
    QVERIFY(cache.lastError().isEmpty());
}

void PersonDQueryTest::repositoryFailuresKeepCurrentError()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QueryCacheRepository openFailure(
        dir.filePath(QStringLiteral("missing-directory/query-cache.sqlite")));
    QVERIFY(!openFailure.open());
    QVERIFY(!openFailure.lastError().isEmpty());

    QueryCacheRepository putFailure(dir.filePath(QStringLiteral("put.sqlite")));
    QVERIFY(!putFailure.put(QStringLiteral("key"), QStringLiteral("[]")));
    QVERIFY(!putFailure.lastError().isEmpty());

    QueryCacheRepository getFailure(dir.filePath(QStringLiteral("get.sqlite")));
    QueryCacheEntry entry;
    QVERIFY(!getFailure.get(QStringLiteral("key"), &entry));
    QVERIFY(!getFailure.lastError().isEmpty());

    QueryCacheRepository removeFailure(dir.filePath(QStringLiteral("remove.sqlite")));
    QVERIFY(!removeFailure.remove(QStringLiteral("key")));
    QVERIFY(!removeFailure.lastError().isEmpty());

    QueryCacheRepository clearFailure(dir.filePath(QStringLiteral("clear.sqlite")));
    QVERIFY(!clearFailure.clear());
    QVERIFY(!clearFailure.lastError().isEmpty());
}

void PersonDQueryTest::rejectsInvalidExamCaches_data()
{
    QTest::addColumn<QString>("payload");

    QTest::newRow("malformed-text") << QStringLiteral("{not-json");
    QTest::newRow("valid-object-wrong-shape") << QStringLiteral("{}");
}

void PersonDQueryTest::rejectsInvalidExamCaches()
{
    QFETCH(QString, payload);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(ExamPlanCacheKey), payload), qPrintable(cache.lastError()));

    FakeZhjwQueryService service;
    service.loggedInValue = false;
    ExamPlanViewModel model(&cache, &service);
    model.load();

    QCOMPARE(model.state(), QueryState::LoginRequired);
    QVERIFY(!model.hasCache());
    QCOMPARE(model.count(), 0);
    QVERIFY(!model.lastUpdated().isValid());
    QCOMPARE(model.errorMessage(), QStringLiteral("考试缓存已损坏，已移除。"));
    QVERIFY(!model.errorMessage().contains(payload));

    QueryCacheEntry entry;
    QVERIFY(!cache.get(QString::fromLatin1(ExamPlanCacheKey), &entry));
    QVERIFY(cache.lastError().isEmpty());
}

void PersonDQueryTest::rejectsInvalidGradeCaches_data()
{
    QTest::addColumn<QString>("cacheKey");
    QTest::addColumn<bool>("schemeCache");
    QTest::addColumn<QString>("payload");

    for (const auto &[name, key, scheme] : {
             std::tuple<const char *, const char *, bool>{"scheme", SchemeCacheKey, true},
             std::tuple<const char *, const char *, bool>{"passing", PassingCacheKey, false}}) {
        QTest::newRow(qPrintable(QStringLiteral("%1-malformed-text").arg(QString::fromLatin1(name))))
            << QString::fromLatin1(key) << scheme << QStringLiteral("{not-json");
        QTest::newRow(qPrintable(QStringLiteral("%1-valid-array-root").arg(QString::fromLatin1(name))))
            << QString::fromLatin1(key) << scheme << QStringLiteral("[]");
        QTest::newRow(qPrintable(QStringLiteral("%1-valid-object-wrong-lnList").arg(QString::fromLatin1(name))))
            << QString::fromLatin1(key) << scheme << QStringLiteral(R"({"lnList":{}})");
    }
}

void PersonDQueryTest::rejectsInvalidGradeCaches()
{
    QFETCH(QString, cacheKey);
    QFETCH(bool, schemeCache);
    QFETCH(QString, payload);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(cacheKey, payload), qPrintable(cache.lastError()));

    FakeZhjwQueryService service;
    service.loggedInValue = false;
    GradesViewModel model(&cache, &service);
    model.load();

    QCOMPARE(model.schemeState(), QueryState::LoginRequired);
    QCOMPARE(model.passingState(), QueryState::LoginRequired);
    QVERIFY(model.schemeGroups().isEmpty());
    QVERIFY(model.passingGroups().isEmpty());
    QVERIFY(!model.schemeLastUpdated().isValid());
    QVERIFY(!model.passingLastUpdated().isValid());
    const QString diagnostic = schemeCache ? model.schemeErrorMessage() : model.passingErrorMessage();
    QCOMPARE(diagnostic, schemeCache
             ? QStringLiteral("方案成绩缓存已损坏，已移除。")
             : QStringLiteral("及格成绩缓存已损坏，已移除。"));
    QVERIFY(!diagnostic.contains(payload));

    QueryCacheEntry entry;
    QVERIFY(!cache.get(cacheKey, &entry));
    QVERIFY(cache.lastError().isEmpty());
}

void PersonDQueryTest::clearExamCacheResetsChangedStateOnce()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));

    FakeZhjwQueryService service;
    service.loggedInValue = true;
    service.examItems = {sampleExamDto()};
    ExamPlanViewModel model(&cache, &service);
    model.load();
    QCOMPARE(model.state(), QueryState::Loaded);
    QCOMPARE(model.count(), 1);
    QVERIFY(model.hasCache());
    QVERIFY(model.lastUpdated().isValid());

    service.nextError = ApiError{
        ApiErrorType::Network, QStringLiteral("考试安排暂时无法刷新"), 503};
    model.refresh();
    QCOMPARE(model.state(), QueryState::Loaded);
    QVERIFY(!model.errorMessage().isEmpty());

    QSignalSpy stateSpy(&model, &ExamPlanViewModel::stateChanged);
    QSignalSpy examsSpy(&model, &ExamPlanViewModel::examsChanged);
    QSignalSpy cacheSpy(&model, &ExamPlanViewModel::cacheChanged);
    QSignalSpy updatedSpy(&model, &ExamPlanViewModel::lastUpdatedChanged);
    QSignalSpy errorSpy(&model, &ExamPlanViewModel::errorChanged);

    model.clearCache();

    QCOMPARE(model.state(), QueryState::Idle);
    QCOMPARE(model.count(), 0);
    QVERIFY(!model.hasCache());
    QVERIFY(!model.lastUpdated().isValid());
    QVERIFY(model.errorMessage().isEmpty());
    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(examsSpy.count(), 1);
    QCOMPARE(cacheSpy.count(), 1);
    QCOMPARE(updatedSpy.count(), 1);
    QCOMPARE(errorSpy.count(), 1);

    QueryCacheEntry entry;
    QVERIFY(!cache.get(QString::fromLatin1(ExamPlanCacheKey), &entry));
    QVERIFY(cache.lastError().isEmpty());

    stateSpy.clear();
    examsSpy.clear();
    cacheSpy.clear();
    updatedSpy.clear();
    errorSpy.clear();
    model.clearCache();
    QCOMPARE(stateSpy.count(), 0);
    QCOMPARE(examsSpy.count(), 0);
    QCOMPARE(cacheSpy.count(), 0);
    QCOMPARE(updatedSpy.count(), 0);
    QCOMPARE(errorSpy.count(), 0);
}

void PersonDQueryTest::clearGradesCacheResetsChangedStateOnce()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));

    FakeZhjwQueryService service;
    service.loggedInValue = true;
    service.schemeScores = sampleSchemeRoot();
    service.passingScores = samplePassingRoot();
    GradesViewModel model(&cache, &service);
    model.load();
    QCOMPARE(model.schemeState(), QueryState::Loaded);
    QCOMPARE(model.passingState(), QueryState::Loaded);
    QVERIFY(model.schemeLastUpdated().isValid());
    QVERIFY(model.passingLastUpdated().isValid());
    QVERIFY(!model.schemeGroups().isEmpty());
    QVERIFY(!model.passingGroups().isEmpty());

    model.setSearchQuery(QStringLiteral("高等"));
    service.nextError = ApiError{
        ApiErrorType::Network, QStringLiteral("方案成绩暂时无法刷新"), 503};
    model.refreshSchemeScores();
    service.nextError = ApiError{
        ApiErrorType::Network, QStringLiteral("及格成绩暂时无法刷新"), 503};
    model.refreshPassingScores();
    QVERIFY(!model.schemeErrorMessage().isEmpty());
    QVERIFY(!model.passingErrorMessage().isEmpty());

    QSignalSpy schemeSpy(&model, &GradesViewModel::schemeChanged);
    QSignalSpy passingSpy(&model, &GradesViewModel::passingChanged);
    QSignalSpy searchSpy(&model, &GradesViewModel::searchQueryChanged);

    model.clearCache();

    QCOMPARE(model.schemeState(), QueryState::Idle);
    QCOMPARE(model.passingState(), QueryState::Idle);
    QVERIFY(model.schemeGroups().isEmpty());
    QVERIFY(model.passingGroups().isEmpty());
    QVERIFY(!model.schemeLastUpdated().isValid());
    QVERIFY(!model.passingLastUpdated().isValid());
    QVERIFY(model.schemeErrorMessage().isEmpty());
    QVERIFY(model.passingErrorMessage().isEmpty());
    QVERIFY(model.searchQuery().isEmpty());
    QCOMPARE(schemeSpy.count(), 1);
    QCOMPARE(passingSpy.count(), 1);
    QCOMPARE(searchSpy.count(), 1);

    QueryCacheEntry entry;
    QVERIFY(!cache.get(QString::fromLatin1(SchemeCacheKey), &entry));
    QVERIFY(cache.lastError().isEmpty());
    QVERIFY(!cache.get(QString::fromLatin1(PassingCacheKey), &entry));
    QVERIFY(cache.lastError().isEmpty());

    schemeSpy.clear();
    passingSpy.clear();
    searchSpy.clear();
    model.clearCache();
    QCOMPARE(schemeSpy.count(), 0);
    QCOMPARE(passingSpy.count(), 0);
    QCOMPARE(searchSpy.count(), 0);
}

void PersonDQueryTest::examCacheRemovalFailureIsVisible()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString databasePath = dir.filePath(QStringLiteral("query-cache.sqlite"));
    QueryCacheRepository cache(databasePath);
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));

    FakeZhjwQueryService service;
    service.loggedInValue = true;
    service.examItems = {sampleExamDto()};
    ExamPlanViewModel model(&cache, &service);
    model.load();
    QCOMPARE(model.state(), QueryState::Loaded);
    QCOMPARE(model.count(), 1);
    QVERIFY(model.hasCache());
    const QDateTime lastUpdated = model.lastUpdated();
    QVERIFY(lastUpdated.isValid());
    QVERIFY(blockCacheDelete(databasePath, QString::fromLatin1(ExamPlanCacheKey)));

    QList<QueryState> observedStates;
    connect(&model, &ExamPlanViewModel::stateChanged, &model, [&] {
        observedStates.append(model.state());
    });
    model.clearCache();

    QCOMPARE(model.state(), QueryState::Error);
    QCOMPARE(model.errorMessage(), QStringLiteral("清除考试缓存失败，请重试。"));
    QCOMPARE(observedStates, QList<QueryState>{QueryState::Error});
    QCOMPARE(model.count(), 1);
    QVERIFY(model.hasCache());
    QCOMPARE(model.lastUpdated(), lastUpdated);
    QCOMPARE(model.exams().first().toMap().value(QStringLiteral("courseName")).toString(),
             QStringLiteral("数据结构"));

    QueryCacheEntry entry;
    QVERIFY(cache.get(QString::fromLatin1(ExamPlanCacheKey), &entry));
}

void PersonDQueryTest::gradesCacheRemovalFailuresAreVisible()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository unopenedCache(dir.filePath(QStringLiteral("query-cache.sqlite")));

    FakeZhjwQueryService service;
    service.loggedInValue = true;
    service.schemeScores = sampleSchemeRoot();
    service.passingScores = samplePassingRoot();
    GradesViewModel model(&unopenedCache, &service);
    model.load();
    QCOMPARE(model.schemeState(), QueryState::Loaded);
    QCOMPARE(model.passingState(), QueryState::Loaded);

    QList<QueryState> observedSchemeStates;
    QList<QueryState> observedPassingStates;
    connect(&model, &GradesViewModel::schemeChanged, &model, [&] {
        observedSchemeStates.append(model.schemeState());
    });
    connect(&model, &GradesViewModel::passingChanged, &model, [&] {
        observedPassingStates.append(model.passingState());
    });
    model.clearCache();

    QCOMPARE(model.schemeState(), QueryState::Error);
    QCOMPARE(model.passingState(), QueryState::Error);
    QCOMPARE(model.schemeErrorMessage(), QStringLiteral("清除方案成绩缓存失败，请重试。"));
    QCOMPARE(model.passingErrorMessage(), QStringLiteral("清除及格成绩缓存失败，请重试。"));
    QCOMPARE(observedSchemeStates, QList<QueryState>{QueryState::Error});
    QCOMPARE(observedPassingStates, QList<QueryState>{QueryState::Error});
}

void PersonDQueryTest::invalidExamCacheRemovalFailureIsSafe()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString databasePath = dir.filePath(QStringLiteral("query-cache.sqlite"));
    QueryCacheRepository cache(databasePath);
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));
    const QString payload = QStringLiteral("{not-json");
    QVERIFY2(cache.put(QString::fromLatin1(ExamPlanCacheKey), payload), qPrintable(cache.lastError()));
    QVERIFY(blockCacheDelete(databasePath, QString::fromLatin1(ExamPlanCacheKey)));

    FakeZhjwQueryService service;
    service.loggedInValue = false;
    ExamPlanViewModel model(&cache, &service);
    QSignalSpy toastSpy(&model, &ExamPlanViewModel::toastRequested);
    QList<QueryState> observedStates;
    connect(&model, &ExamPlanViewModel::stateChanged, &model, [&] {
        observedStates.append(model.state());
    });
    model.load();

    QCOMPARE(model.state(), QueryState::LoginRequired);
    QCOMPARE(observedStates, QList<QueryState>{QueryState::LoginRequired});
    QVERIFY(!model.hasCache());
    QCOMPARE(model.errorMessage(), QStringLiteral("移除损坏的考试缓存失败，请重试。"));
    QVERIFY(!model.errorMessage().contains(payload));
    QVERIFY(!model.errorMessage().contains(QStringLiteral("blocked cache delete")));
    QCOMPARE(toastSpy.count(), 1);
    QCOMPARE(toastSpy.first().first().toString(), model.errorMessage());

    QueryCacheEntry entry;
    QVERIFY(cache.get(QString::fromLatin1(ExamPlanCacheKey), &entry));
}

void PersonDQueryTest::invalidGradeCacheRemovalFailuresAreSafe_data()
{
    QTest::addColumn<QString>("cacheKey");
    QTest::addColumn<bool>("schemeCache");

    QTest::newRow("scheme") << QString::fromLatin1(SchemeCacheKey) << true;
    QTest::newRow("passing") << QString::fromLatin1(PassingCacheKey) << false;
}

void PersonDQueryTest::invalidGradeCacheRemovalFailuresAreSafe()
{
    QFETCH(QString, cacheKey);
    QFETCH(bool, schemeCache);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString databasePath = dir.filePath(QStringLiteral("query-cache.sqlite"));
    QueryCacheRepository cache(databasePath);
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));
    const QString payload = QStringLiteral("{not-json");
    QVERIFY2(cache.put(cacheKey, payload), qPrintable(cache.lastError()));
    QVERIFY(blockCacheDelete(databasePath, cacheKey));

    FakeZhjwQueryService service;
    service.loggedInValue = false;
    GradesViewModel model(&cache, &service);
    QSignalSpy toastSpy(&model, &GradesViewModel::toastRequested);
    QList<QueryState> observedStates;
    if (schemeCache) {
        connect(&model, &GradesViewModel::schemeChanged, &model, [&] {
            observedStates.append(model.schemeState());
        });
    } else {
        connect(&model, &GradesViewModel::passingChanged, &model, [&] {
            observedStates.append(model.passingState());
        });
    }
    model.load();

    QCOMPARE(model.schemeState(), QueryState::LoginRequired);
    QCOMPARE(model.passingState(), QueryState::LoginRequired);
    QCOMPARE(observedStates, QList<QueryState>{QueryState::LoginRequired});
    const QString diagnostic = schemeCache ? model.schemeErrorMessage() : model.passingErrorMessage();
    QCOMPARE(diagnostic, schemeCache
             ? QStringLiteral("移除损坏的方案成绩缓存失败，请重试。")
             : QStringLiteral("移除损坏的及格成绩缓存失败，请重试。"));
    QVERIFY(!diagnostic.contains(payload));
    QVERIFY(!diagnostic.contains(QStringLiteral("blocked cache delete")));
    QCOMPARE(toastSpy.count(), 1);
    QCOMPARE(toastSpy.first().first().toString(), diagnostic);

    QueryCacheEntry entry;
    QVERIFY(cache.get(cacheKey, &entry));
}

void PersonDQueryTest::partialGradeCacheRemovalFailureIsVisible_data()
{
    QTest::addColumn<bool>("schemeRemovalFails");

    QTest::newRow("scheme-fails") << true;
    QTest::newRow("passing-fails") << false;
}

void PersonDQueryTest::partialGradeCacheRemovalFailureIsVisible()
{
    QFETCH(bool, schemeRemovalFails);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString databasePath = dir.filePath(QStringLiteral("query-cache.sqlite"));
    QueryCacheRepository cache(databasePath);
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));

    FakeZhjwQueryService service;
    service.loggedInValue = true;
    service.schemeScores = sampleSchemeRoot();
    service.passingScores = samplePassingRoot();
    GradesViewModel model(&cache, &service);
    model.load();
    QCOMPARE(model.schemeState(), QueryState::Loaded);
    QCOMPARE(model.passingState(), QueryState::Loaded);
    model.setSearchQuery(QStringLiteral("高等"));

    const QString failedKey = QString::fromLatin1(
        schemeRemovalFails ? SchemeCacheKey : PassingCacheKey);
    QVERIFY(blockCacheDelete(databasePath, failedKey));

    QSignalSpy schemeSpy(&model, &GradesViewModel::schemeChanged);
    QSignalSpy passingSpy(&model, &GradesViewModel::passingChanged);
    QSignalSpy toastSpy(&model, &GradesViewModel::toastRequested);
    QList<QueryState> observedSchemeStates;
    QList<QueryState> observedPassingStates;
    connect(&model, &GradesViewModel::schemeChanged, &model, [&] {
        observedSchemeStates.append(model.schemeState());
    });
    connect(&model, &GradesViewModel::passingChanged, &model, [&] {
        observedPassingStates.append(model.passingState());
    });
    model.clearCache();

    QCOMPARE(model.schemeState(), schemeRemovalFails ? QueryState::Error : QueryState::Idle);
    QCOMPARE(model.passingState(), schemeRemovalFails ? QueryState::Idle : QueryState::Error);
    QCOMPARE(observedSchemeStates, QList<QueryState>{model.schemeState()});
    QCOMPARE(observedPassingStates, QList<QueryState>{model.passingState()});
    QCOMPARE(schemeSpy.count(), 1);
    QCOMPARE(passingSpy.count(), 1);
    QCOMPARE(toastSpy.count(), 1);
    QCOMPARE(toastSpy.first().first().toString(), QStringLiteral("清除成绩缓存失败，请重试。"));
    QVERIFY(!model.searchQuery().isEmpty());

    if (schemeRemovalFails) {
        QCOMPARE(model.schemeErrorMessage(), QStringLiteral("清除方案成绩缓存失败，请重试。"));
        QVERIFY(!model.schemeGroups().isEmpty());
        QVERIFY(model.schemeLastUpdated().isValid());
        QVERIFY(model.passingErrorMessage().isEmpty());
        QVERIFY(model.passingGroups().isEmpty());
        QVERIFY(!model.passingLastUpdated().isValid());
    } else {
        QVERIFY(model.schemeErrorMessage().isEmpty());
        QVERIFY(model.schemeGroups().isEmpty());
        QVERIFY(!model.schemeLastUpdated().isValid());
        QCOMPARE(model.passingErrorMessage(), QStringLiteral("清除及格成绩缓存失败，请重试。"));
        QVERIFY(!model.passingGroups().isEmpty());
        QVERIFY(model.passingLastUpdated().isValid());
    }

    QueryCacheEntry entry;
    QVERIFY(cache.get(failedKey, &entry));
    const QString clearedKey = QString::fromLatin1(
        schemeRemovalFails ? PassingCacheKey : SchemeCacheKey);
    QVERIFY(!cache.get(clearedKey, &entry));
    QVERIFY(cache.lastError().isEmpty());
}

void PersonDQueryTest::offlineExamCacheReadFailureClearsInMemoryCache()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString databasePath = dir.filePath(QStringLiteral("query-cache.sqlite"));
    QueryCacheRepository cache(databasePath);
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));

    FakeZhjwQueryService service;
    service.loggedInValue = true;
    service.examItems = {sampleExamDto()};
    ExamPlanViewModel model(&cache, &service);
    model.load();
    QCOMPARE(model.state(), QueryState::Loaded);
    QCOMPARE(model.count(), 1);
    QVERIFY(model.hasCache());
    QVERIFY(model.lastUpdated().isValid());

    QVERIFY(dropCacheTable(databasePath));
    service.loggedInValue = false;
    QSignalSpy stateSpy(&model, &ExamPlanViewModel::stateChanged);
    QSignalSpy examsSpy(&model, &ExamPlanViewModel::examsChanged);
    QSignalSpy cacheSpy(&model, &ExamPlanViewModel::cacheChanged);
    QSignalSpy updatedSpy(&model, &ExamPlanViewModel::lastUpdatedChanged);
    QSignalSpy toastSpy(&model, &ExamPlanViewModel::toastRequested);

    model.load();

    QCOMPARE(model.state(), QueryState::LoginRequired);
    QCOMPARE(model.count(), 0);
    QVERIFY(!model.hasCache());
    QVERIFY(!model.lastUpdated().isValid());
    QCOMPARE(model.errorMessage(), QStringLiteral("读取考试缓存失败，请重试。"));
    QVERIFY(!model.errorMessage().contains(QStringLiteral("no such table"), Qt::CaseInsensitive));
    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(examsSpy.count(), 1);
    QCOMPARE(cacheSpy.count(), 1);
    QCOMPARE(updatedSpy.count(), 1);
    QCOMPARE(toastSpy.count(), 1);
    QCOMPARE(toastSpy.first().first().toString(), model.errorMessage());
}

void PersonDQueryTest::offlineGradeCacheReadFailuresClearInMemoryCaches()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString databasePath = dir.filePath(QStringLiteral("query-cache.sqlite"));
    QueryCacheRepository cache(databasePath);
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));

    FakeZhjwQueryService service;
    service.loggedInValue = true;
    service.schemeScores = sampleSchemeRoot();
    service.passingScores = samplePassingRoot();
    GradesViewModel model(&cache, &service);
    model.load();
    QCOMPARE(model.schemeState(), QueryState::Loaded);
    QCOMPARE(model.passingState(), QueryState::Loaded);
    QVERIFY(!model.schemeGroups().isEmpty());
    QVERIFY(!model.passingGroups().isEmpty());
    QVERIFY(model.schemeLastUpdated().isValid());
    QVERIFY(model.passingLastUpdated().isValid());

    QVERIFY(dropCacheTable(databasePath));
    service.loggedInValue = false;
    QSignalSpy schemeSpy(&model, &GradesViewModel::schemeChanged);
    QSignalSpy passingSpy(&model, &GradesViewModel::passingChanged);
    QSignalSpy toastSpy(&model, &GradesViewModel::toastRequested);

    model.load();

    QCOMPARE(model.schemeState(), QueryState::LoginRequired);
    QCOMPARE(model.passingState(), QueryState::LoginRequired);
    QVERIFY(model.schemeGroups().isEmpty());
    QVERIFY(model.passingGroups().isEmpty());
    QVERIFY(!model.schemeLastUpdated().isValid());
    QVERIFY(!model.passingLastUpdated().isValid());
    QCOMPARE(model.schemeErrorMessage(), QStringLiteral("读取方案成绩缓存失败，请重试。"));
    QCOMPARE(model.passingErrorMessage(), QStringLiteral("读取及格成绩缓存失败，请重试。"));
    QVERIFY(!model.schemeErrorMessage().contains(QStringLiteral("no such table"), Qt::CaseInsensitive));
    QVERIFY(!model.passingErrorMessage().contains(QStringLiteral("no such table"), Qt::CaseInsensitive));
    QCOMPARE(schemeSpy.count(), 1);
    QCOMPARE(passingSpy.count(), 1);
    QCOMPARE(toastSpy.count(), 2);
}

void PersonDQueryTest::onlineGradeCacheReadFailuresTriggerRefresh()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString databasePath = dir.filePath(QStringLiteral("query-cache.sqlite"));
    QueryCacheRepository cache(databasePath);
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));

    DeferredZhjwQueryService service;
    GradesViewModel model(&cache, &service);
    model.load();
    QCOMPARE(service.schemeFetchCount, 1);
    QCOMPARE(service.passingFetchCount, 1);
    service.completeScheme(sampleSchemeRoot());
    service.completePassing(samplePassingRoot());
    QCOMPARE(model.schemeState(), QueryState::Loaded);
    QCOMPARE(model.passingState(), QueryState::Loaded);
    const QDateTime schemeUpdated = model.schemeLastUpdated();
    const QDateTime passingUpdated = model.passingLastUpdated();

    QVERIFY(dropCacheTable(databasePath));
    QSignalSpy toastSpy(&model, &GradesViewModel::toastRequested);
    model.load();

    QCOMPARE(service.schemeFetchCount, 2);
    QCOMPARE(service.passingFetchCount, 2);
    QVERIFY(!model.schemeGroups().isEmpty());
    QVERIFY(!model.passingGroups().isEmpty());
    service.completeScheme({}, ApiError{
        ApiErrorType::Network, QStringLiteral("方案成绩刷新失败"), 503});
    service.completePassing({}, ApiError{
        ApiErrorType::Network, QStringLiteral("及格成绩刷新失败"), 503});
    QCOMPARE(model.schemeState(), QueryState::Loaded);
    QCOMPARE(model.passingState(), QueryState::Loaded);
    QVERIFY(!model.schemeGroups().isEmpty());
    QVERIFY(!model.passingGroups().isEmpty());
    QCOMPARE(model.schemeLastUpdated(), schemeUpdated);
    QCOMPARE(model.passingLastUpdated(), passingUpdated);
    QCOMPARE(toastSpy.count(), 2);
}

void PersonDQueryTest::onlineGradeCacheReadFailureWithoutMemoryFetchesOnce()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString databasePath = dir.filePath(QStringLiteral("query-cache.sqlite"));
    QueryCacheRepository cache(databasePath);
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));
    QVERIFY(dropCacheTable(databasePath));

    SynchronousFailureZhjwQueryService service;
    GradesViewModel model(&cache, &service);
    model.load();

    QCOMPARE(service.schemeFetchCount, 1);
    QCOMPARE(service.passingFetchCount, 1);
    QCOMPARE(model.schemeState(), QueryState::Error);
    QCOMPARE(model.passingState(), QueryState::Error);
    QCOMPARE(model.schemeErrorMessage(), QStringLiteral("同步方案请求失败"));
    QCOMPARE(model.passingErrorMessage(), QStringLiteral("同步及格请求失败"));
    QVERIFY(model.schemeGroups().isEmpty());
    QVERIFY(model.passingGroups().isEmpty());
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
