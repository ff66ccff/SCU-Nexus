#include "src/common/QueryState.h"
#include "src/core/logging/AuthLogger.h"
#include "src/core/network/NetworkSettings.h"
#include "src/models/ExamPlanModels.h"
#include "src/models/GradeModels.h"
#include "src/repositories/QueryCacheRepository.h"
#include "src/services/calendar/AcademicCalendarService.h"
#include "src/services/grades/GradeStatisticsService.h"
#include "src/services/zhjw/ZhjwQueryService.h"
#define private public
#include "src/viewmodels/AcademicCalendarViewModel.h"
#undef private
#include "src/viewmodels/ExamPlanViewModel.h"
#include "src/viewmodels/GradesViewModel.h"

#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>
#include <QUuid>
#include <cstring>
#include <tuple>
#include <utility>

namespace {
constexpr auto ExamPlanCacheKey = "exam_plan.latest";
constexpr auto SchemeCacheKey = "grades.scheme_scores";
constexpr auto PassingCacheKey = "grades.passing_scores";
constexpr auto CalendarEntriesCacheKey = "academic_calendar.entries";
constexpr auto CalendarSelectedCacheKey = "academic_calendar.selected_year";
constexpr auto CalendarLegacyImagesCacheKey = "academic_calendar.images";

QString calendarImagesCacheKey(const QString &path)
{
    return QStringLiteral("academic_calendar.images.%1")
        .arg(QString::fromLatin1(
            QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Sha256).toHex()));
}

QString calendarEntriesPayload(const QList<AcademicCalendarEntry> &entries)
{
    return QString::fromUtf8(
        QJsonDocument(academicCalendarEntriesToJson(entries)).toJson(QJsonDocument::Compact));
}

QString calendarSelectedPayload(const AcademicCalendarEntry &entry)
{
    return QString::fromUtf8(QJsonDocument(QJsonObject{
        {QStringLiteral("path"), entry.path},
        {QStringLiteral("title"), entry.title}
    }).toJson(QJsonDocument::Compact));
}

QString calendarImagesPayload(const QStringList &images)
{
    QJsonArray array;
    for (const QString &image : images) {
        array.append(image);
    }
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

struct StaticNetworkResponse
{
    QByteArray body;
    QByteArray contentType = QByteArrayLiteral("text/html; charset=utf-8");
    int statusCode = 200;
    QNetworkReply::NetworkError networkError = QNetworkReply::NoError;
    bool deferred = false;
};

class StaticNetworkReply : public QNetworkReply
{
public:
    StaticNetworkReply(QNetworkAccessManager::Operation operation,
                       const QNetworkRequest &request,
                       StaticNetworkResponse response,
                       QObject *parent)
        : QNetworkReply(parent),
          m_body(std::move(response.body))
    {
        setOperation(operation);
        setRequest(request);
        setUrl(request.url());
        if (!response.contentType.isEmpty()) {
            setHeader(QNetworkRequest::ContentTypeHeader, response.contentType);
        }
        if (response.statusCode > 0) {
            setAttribute(QNetworkRequest::HttpStatusCodeAttribute, response.statusCode);
        }
        if (response.networkError != QNetworkReply::NoError) {
            setError(response.networkError, QStringLiteral("fixture network failure"));
        }
        open(QIODevice::ReadOnly | QIODevice::Unbuffered);
        if (!response.deferred) {
            QTimer::singleShot(0, this, &StaticNetworkReply::complete);
        }
    }

    void abort() override {}

    void complete()
    {
        if (isFinished()) {
            return;
        }
        setFinished(true);
        emit readyRead();
        emit finished();
    }

    qint64 bytesAvailable() const override
    {
        return m_body.size() - m_offset + QNetworkReply::bytesAvailable();
    }

protected:
    qint64 readData(char *data, qint64 maxSize) override
    {
        if (m_offset >= m_body.size()) {
            return -1;
        }
        const qint64 count = qMin(maxSize, m_body.size() - m_offset);
        std::memcpy(data, m_body.constData() + m_offset, static_cast<size_t>(count));
        m_offset += count;
        return count;
    }

private:
    QByteArray m_body;
    qint64 m_offset = 0;
};

class StaticNetworkAccessManager : public QNetworkAccessManager
{
public:
    QList<StaticNetworkResponse> responses;
    QList<QNetworkRequest> requests;
    QList<StaticNetworkReply *> replies;

protected:
    QNetworkReply *createRequest(Operation operation,
                                 const QNetworkRequest &request,
                                 QIODevice *outgoingData) override
    {
        Q_UNUSED(outgoingData)
        requests.append(request);
        Q_ASSERT(!responses.isEmpty());
        auto *reply = new StaticNetworkReply(operation, request, responses.takeFirst(), this);
        replies.append(reply);
        return reply;
    }
};

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

bool unblockCacheDelete(const QString &databasePath)
{
    return executeCacheSql(databasePath, QStringLiteral("DROP TRIGGER block_cache_delete"));
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
    void decodesQuotedGb18030CalendarCharset();
    void decodesQuotedUtf8CalendarCharset();
    void fallsBackToGb18030WithoutDeclaredCharset();
    void calendarRequestsUseSharedTransportPolicy();
    void calendarServiceClassifiesFailures_data();
    void calendarServiceClassifiesFailures();
    void calendarEntriesDistinguishExplicitEmptyAndParseFailure();
    void calendarDetailDistinguishesExplicitEmptyAndParseFailure();
    void calendarParseFailureSummaryIsSafe();
    void calendarViewModelMapsTypedFailuresWithoutCache_data();
    void calendarViewModelMapsTypedFailuresWithoutCache();
    void calendarViewModelRestoresCacheOnTypedFailure_data();
    void calendarViewModelRestoresCacheOnTypedFailure();
    void calendarViewModelPreservesExplicitEmptyCache();
    void calendarExplicitEmptyListClearsPriorDetail();
    void calendarRefreshIgnoresStaleDetail_data();
    void calendarRefreshIgnoresStaleDetail();
    void calendarImageCachesAreEntrySpecific();
    void calendarEntrySwitchRestoresListTimestamp_data();
    void calendarEntrySwitchRestoresListTimestamp();
    void calendarSelectionSurvivesReorderedEntries();
    void calendarRejectsMalformedCaches_data();
    void calendarRejectsMalformedCaches();
    void calendarMalformedEntriesCleanLegacyState_data();
    void calendarMalformedEntriesCleanLegacyState();
    void calendarKeepsValidEmptyImageCache();
    void calendarMigratesLegacySelectionAndImages();
    void calendarDoesNotMigrateLegacyImagesToDuplicateTitle();
    void calendarDropsUnmatchedLegacySelectionAndImages();
    void calendarCachePutFailuresKeepNetworkData();
    void calendarMalformedCacheRemovalFailureIsSafe();
    void calendarClearRemovesKnownKeysAndResetsState();
    void calendarClearRemovesStaleSelectedPathImages();
    void calendarClearPartialFailureKeepsDataAndCanRetry();
    void calendarCachedRequestsRefreshAndRetainOnFailure_data();
    void calendarCachedRequestsRefreshAndRetainOnFailure();
    void calendarEmptyCachesAndVisibleEntriesRefreshSafely_data();
    void calendarEmptyCachesAndVisibleEntriesRefreshSafely();
    void calendarLoadWithoutCacheStartsListRequest();
    void calendarImageViewerShowsLoadingAndFailureFallback();
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
    void cachedExamRefreshUsesRefreshingAndKeepsContent();
    void cachedSchemeRefreshUsesRefreshingAndKeepsContent();
    void cachedPassingRefreshUsesRefreshingAndKeepsContent();
    void emptyCachedSchemeRefreshFailureReturnsEmpty();
    void emptyCachedGradeRefreshWhileLoggedOutStaysEmpty_data();
    void emptyCachedGradeRefreshWhileLoggedOutStaysEmpty();
    void noCacheRefreshFailureStates_data();
    void noCacheRefreshFailureStates();
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

void PersonDQueryTest::decodesQuotedGb18030CalendarCharset()
{
    const QByteArray bytes = QByteArray::fromHex("cbc4b4a8b4f3d1a7d0a3c0fa95328236");

    QCOMPARE(AcademicCalendarService::decodeHtml(
                 bytes, QByteArrayLiteral("text/html; charset=\"GB18030\"; boundary=x")),
             QStringLiteral("四川大学校历𠀀"));
}

void PersonDQueryTest::decodesQuotedUtf8CalendarCharset()
{
    const QString html = QStringLiteral("<p>四川大学校历</p>");

    QCOMPARE(AcademicCalendarService::decodeHtml(
                 html.toUtf8(), QByteArrayLiteral("text/html; charset='utf-8'; boundary=x")),
             html);
}

void PersonDQueryTest::fallsBackToGb18030WithoutDeclaredCharset()
{
    const QByteArray bytes = QByteArray::fromHex("d0a3c0fa95328236");

    QCOMPARE(AcademicCalendarService::decodeHtml(bytes), QStringLiteral("校历𠀀"));
}

void PersonDQueryTest::calendarRequestsUseSharedTransportPolicy()
{
    const QUrl helperUrl(QStringLiteral("https://example.invalid/calendar"));
    const QNetworkRequest built = AcademicCalendarService::buildRequest(helperUrl);
    QCOMPARE(built.url(), helperUrl);
    QCOMPARE(built.rawHeader("User-Agent"), NetworkSettings::kDefaultUserAgent.toUtf8());
    QCOMPARE(built.rawHeader("Accept"), QByteArrayLiteral("text/html,application/xhtml+xml"));
    QCOMPARE(built.transferTimeout(), NetworkSettings::kDefaultTimeoutMs);

    StaticNetworkAccessManager network;
    network.responses = {
        {QStringLiteral("暂无校历数据").toUtf8()},
        {QStringLiteral("暂无数据").toUtf8()}
    };
    AcademicCalendarService service(&network);
    int entriesFetched = 0;
    int detailFetched = 0;
    connect(&service, &AcademicCalendarService::entriesFetched, this,
            [&entriesFetched](const QList<AcademicCalendarEntry> &) { ++entriesFetched; });
    connect(&service, &AcademicCalendarService::detailFetched, this,
            [&detailFetched](const AcademicCalendarDetail &) { ++detailFetched; });

    service.fetchEntries();
    QTRY_COMPARE(entriesFetched, 1);
    service.fetchDetail({QStringLiteral("2025-2026"), QStringLiteral("info/1101/1234.htm")});
    QTRY_COMPARE(detailFetched, 1);
    QCOMPARE(network.requests.size(), 2);
    for (const QNetworkRequest &request : std::as_const(network.requests)) {
        QCOMPARE(request.rawHeader("User-Agent"), NetworkSettings::kDefaultUserAgent.toUtf8());
        QCOMPARE(request.rawHeader("Accept"), QByteArrayLiteral("text/html,application/xhtml+xml"));
        QCOMPARE(request.transferTimeout(), NetworkSettings::kDefaultTimeoutMs);
    }
}

void PersonDQueryTest::calendarServiceClassifiesFailures_data()
{
    QTest::addColumn<int>("networkError");
    QTest::addColumn<int>("statusCode");
    QTest::addColumn<int>("expectedType");

    QTest::newRow("qt-timeout")
        << static_cast<int>(QNetworkReply::TimeoutError) << 0
        << static_cast<int>(ApiErrorType::Timeout);
    QTest::newRow("qt-transfer-timeout-cancel")
        << static_cast<int>(QNetworkReply::OperationCanceledError) << 0
        << static_cast<int>(ApiErrorType::Timeout);
    QTest::newRow("qt-network")
        << static_cast<int>(QNetworkReply::HostNotFoundError) << 0
        << static_cast<int>(ApiErrorType::Network);
    QTest::newRow("http-401")
        << static_cast<int>(QNetworkReply::AuthenticationRequiredError) << 401
        << static_cast<int>(ApiErrorType::Unauthenticated);
    QTest::newRow("http-403")
        << static_cast<int>(QNetworkReply::ContentAccessDenied) << 403
        << static_cast<int>(ApiErrorType::Unauthenticated);
    QTest::newRow("http-429")
        << static_cast<int>(QNetworkReply::UnknownContentError) << 429
        << static_cast<int>(ApiErrorType::RateLimited);
    QTest::newRow("http-503")
        << static_cast<int>(QNetworkReply::ServiceUnavailableError) << 503
        << static_cast<int>(ApiErrorType::ServiceUnavailable);
}

void PersonDQueryTest::calendarServiceClassifiesFailures()
{
    QFETCH(int, networkError);
    QFETCH(int, statusCode);
    QFETCH(int, expectedType);

    StaticNetworkAccessManager network;
    network.responses.append({
        QByteArrayLiteral("token=must-not-be-read studentId=202612345678"),
        QByteArrayLiteral("text/html; charset=utf-8"),
        statusCode,
        static_cast<QNetworkReply::NetworkError>(networkError)
    });
    AcademicCalendarService service(&network);
    QList<ApiError> errors;
    connect(&service, &AcademicCalendarService::failed, this,
            [&errors](const ApiError &error) { errors.append(error); });

    service.fetchEntries();

    QTRY_COMPARE(errors.size(), 1);
    QCOMPARE(errors.first().type, static_cast<ApiErrorType>(expectedType));
    QCOMPARE(errors.first().statusCode, statusCode);
    QVERIFY(errors.first().debugBody.isEmpty());
    QVERIFY(!errors.first().message.contains(QStringLiteral("202612345678")));
    QVERIFY(!errors.first().message.contains(QStringLiteral("must-not-be-read")));
}

void PersonDQueryTest::calendarEntriesDistinguishExplicitEmptyAndParseFailure()
{
    QVERIFY(AcademicCalendarService::calendarPageExplicitlyEmpty(
        QStringLiteral("<p>暂无校历数据</p>")));
    QVERIFY(AcademicCalendarService::calendarPageExplicitlyEmpty(
        QStringLiteral("<p>暂无数据</p>")));
    QVERIFY(AcademicCalendarService::calendarPageExplicitlyEmpty(
        QStringLiteral("<p>没有查询到相关内容</p>")));
    QVERIFY(!AcademicCalendarService::calendarPageExplicitlyEmpty(
        QStringLiteral("<html><title>统一认证</title><p>欢迎访问</p></html>")));

    StaticNetworkAccessManager network;
    network.responses = {
        {QStringLiteral("<p>暂无校历数据</p>").toUtf8()},
        {QStringLiteral("<html><title>统一认证</title><p>欢迎访问</p></html>").toUtf8()}
    };
    AcademicCalendarService service(&network);
    QList<QList<AcademicCalendarEntry>> results;
    QList<ApiError> errors;
    connect(&service, &AcademicCalendarService::entriesFetched, this,
            [&results](const QList<AcademicCalendarEntry> &entries) { results.append(entries); });
    connect(&service, &AcademicCalendarService::failed, this,
            [&errors](const ApiError &error) { errors.append(error); });

    service.fetchEntries();
    QTRY_COMPARE(results.size(), 1);
    QVERIFY(results.first().isEmpty());
    QVERIFY(errors.isEmpty());

    service.fetchEntries();
    QTRY_COMPARE(errors.size(), 1);
    QCOMPARE(errors.first().type, ApiErrorType::ParseFailed);
    QCOMPARE(results.size(), 1);
}

void PersonDQueryTest::calendarDetailDistinguishesExplicitEmptyAndParseFailure()
{
    const AcademicCalendarEntry entry{
        QStringLiteral("2025-2026"), QStringLiteral("info/1101/1234.htm")};
    StaticNetworkAccessManager network;
    network.responses = {
        {QStringLiteral("<p>没有查询到校历图片</p>").toUtf8()},
        {QStringLiteral("<html><title>网站维护中</title></html>").toUtf8()}
    };
    AcademicCalendarService service(&network);
    QList<AcademicCalendarDetail> details;
    QList<ApiError> errors;
    connect(&service, &AcademicCalendarService::detailFetched, this,
            [&details](const AcademicCalendarDetail &detail) { details.append(detail); });
    connect(&service, &AcademicCalendarService::failed, this,
            [&errors](const ApiError &error) { errors.append(error); });

    service.fetchDetail(entry);
    QTRY_COMPARE(details.size(), 1);
    QVERIFY(details.first().imageUrls.isEmpty());
    QVERIFY(errors.isEmpty());

    service.fetchDetail(entry);
    QTRY_COMPARE(errors.size(), 1);
    QCOMPARE(errors.first().type, ApiErrorType::ParseFailed);
    QCOMPARE(details.size(), 1);
}

void PersonDQueryTest::calendarParseFailureSummaryIsSafe()
{
    const QString secretToken = QStringLiteral("calendar-secret-token");
    const QString studentNumber = QStringLiteral("202612345678");
    const QString body = QStringLiteral(
        "<html>\n\t token=%1 student=%2 Cookie: SID=cookie-secret </html> %3")
        .arg(secretToken, studentNumber, QString(900, QLatin1Char('x')));
    StaticNetworkAccessManager network;
    network.responses.append({body.toUtf8()});
    AcademicCalendarService service(&network);
    QList<ApiError> errors;
    connect(&service, &AcademicCalendarService::failed, this,
            [&errors](const ApiError &error) { errors.append(error); });
    AuthLogger::instance().clear();

    service.fetchEntries();

    QTRY_COMPARE(errors.size(), 1);
    const ApiError error = errors.first();
    QCOMPARE(error.type, ApiErrorType::ParseFailed);
    QVERIFY(error.debugBody.size() <= 500);
    QVERIFY(!error.debugBody.contains(secretToken));
    QVERIFY(!error.debugBody.contains(studentNumber));
    QVERIFY(!error.debugBody.contains(QStringLiteral("cookie-secret")));
    QVERIFY(!error.debugBody.contains(QLatin1Char('\n')));
    QVERIFY(!error.debugBody.contains(QLatin1Char('\t')));
    QVERIFY(!error.debugBody.contains(QStringLiteral("  ")));
    QVERIFY(error.debugBody.size() < body.size());

    const QList<AuthLogEntry> entries = AuthLogger::instance().entries();
    QCOMPARE(entries.size(), 1);
    QVERIFY(!entries.first().message.contains(secretToken));
    QVERIFY(!entries.first().message.contains(studentNumber));
    QVERIFY(!entries.first().message.contains(QStringLiteral("cookie-secret")));
    QVERIFY(!entries.first().message.contains(body));
}

void PersonDQueryTest::calendarViewModelMapsTypedFailuresWithoutCache_data()
{
    QTest::addColumn<int>("errorType");
    QTest::addColumn<int>("expectedState");

    QTest::newRow("unauthenticated")
        << static_cast<int>(ApiErrorType::Unauthenticated)
        << static_cast<int>(QueryState::LoginRequired);
    QTest::newRow("session-expired")
        << static_cast<int>(ApiErrorType::SessionExpired)
        << static_cast<int>(QueryState::LoginRequired);
    QTest::newRow("network")
        << static_cast<int>(ApiErrorType::Network)
        << static_cast<int>(QueryState::Error);
    QTest::newRow("parse")
        << static_cast<int>(ApiErrorType::ParseFailed)
        << static_cast<int>(QueryState::Error);
}

void PersonDQueryTest::calendarViewModelMapsTypedFailuresWithoutCache()
{
    QFETCH(int, errorType);
    QFETCH(int, expectedState);

    AcademicCalendarViewModel model(nullptr);
    QSignalSpy toastSpy(&model, &AcademicCalendarViewModel::toastRequested);
    model.setState(QueryState::Loading);
    const QString message = QStringLiteral("校历安全错误提示");

    model.m_service.failed({static_cast<ApiErrorType>(errorType), message, 503});

    QCOMPARE(model.state(), static_cast<QueryState>(expectedState));
    QCOMPARE(model.errorMessage(), message);
    QVERIFY(!model.hasCache());
    QVERIFY(model.entries().isEmpty());
    QVERIFY(!model.lastUpdated().isValid());
    QCOMPARE(toastSpy.count(), 0);
}

void PersonDQueryTest::calendarViewModelRestoresCacheOnTypedFailure_data()
{
    QTest::addColumn<QStringList>("images");
    QTest::addColumn<int>("expectedState");

    QTest::newRow("cached-empty")
        << QStringList{} << static_cast<int>(QueryState::Empty);
    QTest::newRow("cached-loaded")
        << QStringList{QStringLiteral("https://example.invalid/calendar.png")}
        << static_cast<int>(QueryState::Loaded);
}

void PersonDQueryTest::calendarViewModelRestoresCacheOnTypedFailure()
{
    QFETCH(QStringList, images);
    QFETCH(int, expectedState);

    AcademicCalendarViewModel model(nullptr);
    model.m_hasCache = true;
    model.m_entries = {{QStringLiteral("2025-2026"), QStringLiteral("info/1101/1234.htm")}};
    model.m_selectedIndex = 0;
    model.m_imageUrls = images;
    model.m_lastUpdated = QDateTime::fromString(
        QStringLiteral("2025-01-02T03:04:05.000Z"), Qt::ISODateWithMs);
    model.setState(QueryState::Refreshing);
    const QVariantList preservedEntries = model.entries();
    const QStringList preservedImages = model.imageUrls();
    const QDateTime preservedTimestamp = model.lastUpdated();
    QSignalSpy toastSpy(&model, &AcademicCalendarViewModel::toastRequested);
    const QString message = QStringLiteral("校历刷新失败，继续显示缓存");

    model.m_service.failed({ApiErrorType::ParseFailed, message, 200});

    QCOMPARE(model.state(), static_cast<QueryState>(expectedState));
    QCOMPARE(model.entries(), preservedEntries);
    QCOMPARE(model.imageUrls(), preservedImages);
    QCOMPARE(model.lastUpdated(), preservedTimestamp);
    QVERIFY(model.hasCache());
    QCOMPARE(toastSpy.count(), 1);
    QCOMPARE(toastSpy.first().first().toString(), message);
}

void PersonDQueryTest::calendarViewModelPreservesExplicitEmptyCache()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));
    AcademicCalendarViewModel writer(&cache);
    QSignalSpy writerToastSpy(&writer, &AcademicCalendarViewModel::toastRequested);

    writer.applyEntries({}, true);

    QCOMPARE(writer.state(), QueryState::Empty);
    QVERIFY(writer.hasCache());
    QVERIFY(writer.entries().isEmpty());
    QVERIFY(writer.lastUpdated().isValid());
    QCOMPARE(writerToastSpy.count(), 0);

    QueryCacheEntry cacheEntry;
    QVERIFY2(cache.get(QStringLiteral("academic_calendar.entries"), &cacheEntry),
             qPrintable(cache.lastError()));
    QCOMPARE(cacheEntry.payloadJson, QStringLiteral("[]"));

    AcademicCalendarViewModel reader(&cache);
    reader.readCache();
    QVERIFY(reader.hasCache());
    QVERIFY(reader.entries().isEmpty());
    const QDateTime cachedAt = reader.lastUpdated();
    QVERIFY(cachedAt.isValid());
    reader.setState(QueryState::Refreshing);
    QSignalSpy toastSpy(&reader, &AcademicCalendarViewModel::toastRequested);

    reader.m_service.failed({ApiErrorType::Network, QStringLiteral("离线"), 0});

    QCOMPARE(reader.state(), QueryState::Empty);
    QVERIFY(reader.entries().isEmpty());
    QCOMPARE(reader.lastUpdated(), cachedAt);
    QCOMPARE(toastSpy.count(), 1);
}

void PersonDQueryTest::calendarExplicitEmptyListClearsPriorDetail()
{
    const AcademicCalendarEntry oldEntry{
        QStringLiteral("2024-2025"), QStringLiteral("info/1101/1111.htm")};
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(calendarImagesCacheKey(oldEntry.path),
                       QStringLiteral("[\"https://example.invalid/old-calendar.png\"]")),
             qPrintable(cache.lastError()));
    AcademicCalendarViewModel model(&cache);
    model.m_entries = {oldEntry};
    model.m_selectedIndex = 0;
    model.m_imageUrls = {QStringLiteral("https://example.invalid/old-calendar.png")};
    model.m_hasCache = true;
    model.m_lastUpdated = QDateTime::fromString(
        QStringLiteral("2025-01-02T03:04:05.000Z"), Qt::ISODateWithMs);

    model.applyEntries({}, true);

    QCOMPARE(model.state(), QueryState::Empty);
    QVERIFY(model.entries().isEmpty());
    QCOMPARE(model.selectedIndex(), -1);
    QVERIFY(model.imageUrls().isEmpty());
    QVERIFY(model.hasCache());
    QVERIFY(model.lastUpdated()
            > QDateTime::fromString(QStringLiteral("2025-01-02T03:04:05.000Z"),
                                    Qt::ISODateWithMs));

    QueryCacheEntry imageCache;
    QVERIFY2(cache.get(calendarImagesCacheKey(oldEntry.path), &imageCache),
             qPrintable(cache.lastError()));
    QCOMPARE(imageCache.payloadJson, QStringLiteral("[]"));

    AcademicCalendarViewModel reader(&cache);
    reader.readCache();
    QVERIFY(reader.hasCache());
    QVERIFY(reader.entries().isEmpty());
    QCOMPARE(reader.selectedIndex(), -1);
    QVERIFY(reader.imageUrls().isEmpty());

    model.setState(QueryState::Refreshing);
    model.m_service.failed({ApiErrorType::Network, QStringLiteral("离线"), 0});
    QCOMPARE(model.state(), QueryState::Empty);
}

void PersonDQueryTest::calendarRefreshIgnoresStaleDetail_data()
{
    QTest::addColumn<bool>("detailFails");

    QTest::newRow("success") << false;
    QTest::newRow("failure") << true;
}

void PersonDQueryTest::calendarRefreshIgnoresStaleDetail()
{
    QFETCH(bool, detailFails);

    const AcademicCalendarEntry staleEntry{
        QStringLiteral("2024-2025"), QStringLiteral("info/1101/1111.htm")};
    StaticNetworkResponse staleDetail{
        detailFails
            ? QByteArray{}
            : QStringLiteral("<img src=\"/__local/old/calendar.png\">").toUtf8(),
        QByteArrayLiteral("text/html; charset=utf-8"),
        detailFails ? 0 : 200,
        detailFails ? QNetworkReply::HostNotFoundError : QNetworkReply::NoError,
        true
    };
    StaticNetworkAccessManager network;
    network.responses = {
        staleDetail,
        {QStringLiteral("<p>暂无校历数据</p>").toUtf8()}
    };
    AcademicCalendarService service(&network);
    AcademicCalendarViewModel model(nullptr);
    model.m_entries = {staleEntry};
    model.m_selectedIndex = 0;
    model.m_imageUrls = {QStringLiteral("https://example.invalid/old-calendar.png")};
    model.m_hasCache = true;
    model.setState(QueryState::Loaded);

    connect(&service, &AcademicCalendarService::entriesFetched,
            &model.m_service, &AcademicCalendarService::entriesFetched);
    connect(&service, &AcademicCalendarService::detailFetched,
            &model.m_service, &AcademicCalendarService::detailFetched);
    connect(&service, &AcademicCalendarService::failed,
            &model.m_service, &AcademicCalendarService::failed);
    int detailSignalCount = 0;
    int failureSignalCount = 0;
    connect(&service, &AcademicCalendarService::detailFetched, this,
            [&detailSignalCount](const AcademicCalendarDetail &) { ++detailSignalCount; });
    connect(&service, &AcademicCalendarService::failed, this,
            [&failureSignalCount](const ApiError &) { ++failureSignalCount; });
    QSignalSpy toastSpy(&model, &AcademicCalendarViewModel::toastRequested);
    QSignalSpy errorSpy(&model, &AcademicCalendarViewModel::errorChanged);

    service.fetchDetail(staleEntry);
    QCOMPARE(network.replies.size(), 1);
    StaticNetworkReply *pendingDetail = network.replies.first();
    service.fetchEntries();

    QTRY_COMPARE(model.state(), QueryState::Empty);
    QVERIFY(model.entries().isEmpty());
    QCOMPARE(model.selectedIndex(), -1);
    QVERIFY(model.imageUrls().isEmpty());

    pendingDetail->complete();

    QCOMPARE(model.state(), QueryState::Empty);
    QVERIFY(model.entries().isEmpty());
    QCOMPARE(model.selectedIndex(), -1);
    QVERIFY(model.imageUrls().isEmpty());
    QVERIFY(model.errorMessage().isEmpty());
    QCOMPARE(errorSpy.count(), 0);
    QCOMPARE(toastSpy.count(), 0);
    QCOMPARE(detailSignalCount, 0);
    QCOMPARE(failureSignalCount, 0);
}

void PersonDQueryTest::calendarImageCachesAreEntrySpecific()
{
    const AcademicCalendarEntry first{
        QStringLiteral("2024-2025"), QStringLiteral("info/1101/1111.htm")};
    const AcademicCalendarEntry second{
        QStringLiteral("2025-2026"), QStringLiteral("info/1101/2222.htm")};
    const QStringList firstImages{QStringLiteral("https://example.invalid/first.png")};
    const QStringList secondImages{QStringLiteral("https://example.invalid/second.png")};

    QCOMPARE(AcademicCalendarViewModel::imagesCacheKey(first.path),
             QStringLiteral("academic_calendar.images."
                            "b5e5e517605bb052d4e450f2091afca2c8a6f886e92a76badff039e9d7eefb38"));
    QVERIFY(AcademicCalendarViewModel::imagesCacheKey(first.path)
            != AcademicCalendarViewModel::imagesCacheKey(second.path));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarEntriesCacheKey),
                       calendarEntriesPayload({first, second})), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarSelectedCacheKey),
                       calendarSelectedPayload(second)), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(calendarImagesCacheKey(first.path),
                       calendarImagesPayload(firstImages)), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(calendarImagesCacheKey(second.path),
                       calendarImagesPayload(secondImages)), qPrintable(cache.lastError()));

    StaticNetworkAccessManager network;
    network.responses = {
        {QByteArray{}, QByteArrayLiteral("text/html; charset=utf-8"), 200,
         QNetworkReply::NoError, true},
        {QByteArray{}, QByteArrayLiteral("text/html; charset=utf-8"), 200,
         QNetworkReply::NoError, true}
    };
    AcademicCalendarService service(&network);
    AcademicCalendarViewModel model(&cache, &service, nullptr);

    model.readCache();

    QCOMPARE(model.selectedIndex(), 1);
    QCOMPARE(model.imageUrls(), secondImages);

    model.selectEntry(0);
    QCOMPARE(model.selectedIndex(), 0);
    QCOMPARE(model.imageUrls(), firstImages);

    model.selectEntry(1);
    QCOMPARE(model.selectedIndex(), 1);
    QCOMPARE(model.imageUrls(), secondImages);
    QCOMPARE(network.requests.size(), 2);
}

void PersonDQueryTest::calendarEntrySwitchRestoresListTimestamp_data()
{
    QTest::addColumn<QString>("secondImagePayload");

    QTest::newRow("uncached") << QString();
    QTest::newRow("malformed-cache") << QStringLiteral("{not-json");
}

void PersonDQueryTest::calendarEntrySwitchRestoresListTimestamp()
{
    QFETCH(QString, secondImagePayload);

    const AcademicCalendarEntry first{
        QStringLiteral("2024-2025"), QStringLiteral("info/1101/1111.htm")};
    const AcademicCalendarEntry second{
        QStringLiteral("2025-2026"), QStringLiteral("info/1101/2222.htm")};
    const QDateTime entriesAt = QDateTime::fromString(
        QStringLiteral("2026-01-02T03:04:05.000Z"), Qt::ISODateWithMs);
    const QDateTime firstImagesAt = entriesAt.addDays(1);
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarEntriesCacheKey),
                       calendarEntriesPayload({first, second}), entriesAt),
             qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarSelectedCacheKey),
                       calendarSelectedPayload(first), entriesAt), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(calendarImagesCacheKey(first.path),
                       calendarImagesPayload({QStringLiteral("https://example.invalid/first.png")}),
                       firstImagesAt), qPrintable(cache.lastError()));
    if (!secondImagePayload.isNull()) {
        QVERIFY2(cache.put(calendarImagesCacheKey(second.path), secondImagePayload,
                           firstImagesAt.addDays(1)), qPrintable(cache.lastError()));
    }

    StaticNetworkAccessManager network;
    network.responses.append({QByteArray{}, QByteArrayLiteral("text/html; charset=utf-8"), 200,
                              QNetworkReply::NoError, true});
    AcademicCalendarService service(&network);
    AcademicCalendarViewModel model(&cache, &service, nullptr);
    model.readCache();
    QCOMPARE(model.lastUpdated(), firstImagesAt);
    QSignalSpy updatedSpy(&model, &AcademicCalendarViewModel::lastUpdatedChanged);

    model.selectEntry(1);

    QCOMPARE(model.selectedIndex(), 1);
    QVERIFY(model.imageUrls().isEmpty());
    QCOMPARE(model.lastUpdated(), entriesAt);
    QCOMPARE(updatedSpy.count(), 1);
    QCOMPARE(model.state(), QueryState::Refreshing);
    QCOMPARE(network.requests.size(), 1);
    if (!secondImagePayload.isNull()) {
        QueryCacheEntry removed;
        QVERIFY(!cache.get(calendarImagesCacheKey(second.path), &removed));
    }
}

void PersonDQueryTest::calendarSelectionSurvivesReorderedEntries()
{
    const AcademicCalendarEntry first{
        QStringLiteral("2024-2025"), QStringLiteral("info/1101/1111.htm")};
    const AcademicCalendarEntry second{
        QStringLiteral("2025-2026"), QStringLiteral("info/1101/2222.htm")};
    const AcademicCalendarEntry renamedPath{
        second.title, QStringLiteral("info/1101/3333.htm")};
    const AcademicCalendarEntry fallback{
        QStringLiteral("2026-2027"), QStringLiteral("info/1101/4444.htm")};

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarEntriesCacheKey),
                       calendarEntriesPayload({first, second})), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarSelectedCacheKey),
                       calendarSelectedPayload(second)), qPrintable(cache.lastError()));

    StaticNetworkAccessManager network;
    for (int i = 0; i < 3; ++i) {
        network.responses.append({QByteArray{}, QByteArrayLiteral("text/html; charset=utf-8"),
                                  200, QNetworkReply::NoError, true});
    }
    AcademicCalendarService service(&network);
    AcademicCalendarViewModel model(&cache, &service, nullptr);
    model.readCache();

    model.applyEntries({second, first}, true);
    QCOMPARE(model.selectedIndex(), 0);
    QCOMPARE(model.entries().at(model.selectedIndex()).toMap().value(QStringLiteral("path")).toString(),
             second.path);

    model.applyEntries({first, renamedPath}, true);
    QCOMPARE(model.selectedIndex(), 1);
    QCOMPARE(model.entries().at(model.selectedIndex()).toMap().value(QStringLiteral("path")).toString(),
             renamedPath.path);

    model.applyEntries({fallback, first}, true);
    QCOMPARE(model.selectedIndex(), 0);
    QCOMPARE(model.entries().at(model.selectedIndex()).toMap().value(QStringLiteral("path")).toString(),
             fallback.path);

    QueryCacheEntry selectedCache;
    QVERIFY2(cache.get(QString::fromLatin1(CalendarSelectedCacheKey), &selectedCache),
             qPrintable(cache.lastError()));
    QJsonParseError parseError;
    const QJsonDocument selectedDocument = QJsonDocument::fromJson(
        selectedCache.payloadJson.toUtf8(), &parseError);
    QCOMPARE(parseError.error, QJsonParseError::NoError);
    QCOMPARE(selectedDocument.object().value(QStringLiteral("title")).toString(), fallback.title);
    QCOMPARE(selectedDocument.object().value(QStringLiteral("path")).toString(), fallback.path);
}

void PersonDQueryTest::calendarRejectsMalformedCaches_data()
{
    QTest::addColumn<int>("cacheKind");
    QTest::addColumn<QString>("payload");

    QTest::newRow("entries-malformed") << 0 << QStringLiteral("{not-json");
    QTest::newRow("entries-wrong-root") << 0 << QStringLiteral("{}");
    QTest::newRow("entries-wrong-item")
        << 0 << QStringLiteral("[{\"title\":\"2025-2026\",\"path\":7}]");
    QTest::newRow("selected-malformed") << 1 << QStringLiteral("[1]");
    QTest::newRow("selected-wrong-field")
        << 1 << QStringLiteral("{\"title\":\"2025-2026\",\"path\":7}");
    QTest::newRow("images-malformed") << 2 << QStringLiteral("{not-json");
    QTest::newRow("images-wrong-root") << 2 << QStringLiteral("{}");
    QTest::newRow("images-wrong-item")
        << 2 << QStringLiteral("[\"https://example.invalid/calendar.png\",7]");
}

void PersonDQueryTest::calendarRejectsMalformedCaches()
{
    QFETCH(int, cacheKind);
    QFETCH(QString, payload);

    const AcademicCalendarEntry entry{
        QStringLiteral("2025-2026"), QStringLiteral("info/1101/2222.htm")};
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));

    const QString targetKey = cacheKind == 0
        ? QString::fromLatin1(CalendarEntriesCacheKey)
        : (cacheKind == 1 ? QString::fromLatin1(CalendarSelectedCacheKey)
                          : calendarImagesCacheKey(entry.path));
    if (cacheKind != 0) {
        QVERIFY2(cache.put(QString::fromLatin1(CalendarEntriesCacheKey),
                           calendarEntriesPayload({entry})), qPrintable(cache.lastError()));
    }
    if (cacheKind == 2) {
        QVERIFY2(cache.put(QString::fromLatin1(CalendarSelectedCacheKey),
                           calendarSelectedPayload(entry)), qPrintable(cache.lastError()));
    }
    QVERIFY2(cache.put(targetKey, payload), qPrintable(cache.lastError()));

    AcademicCalendarViewModel model(&cache);
    model.readCache();

    QueryCacheEntry damaged;
    QVERIFY(!cache.get(targetKey, &damaged));
    QVERIFY(cache.lastError().isEmpty());
    if (cacheKind == 0) {
        QVERIFY(model.entries().isEmpty());
        QVERIFY(!model.hasCache());
    } else if (cacheKind == 1) {
        QCOMPARE(model.selectedIndex(), 0);
    } else {
        QVERIFY(model.imageUrls().isEmpty());
    }
}

void PersonDQueryTest::calendarMalformedEntriesCleanLegacyState_data()
{
    QTest::addColumn<QString>("blockedKey");

    QTest::newRow("all-removals-succeed") << QString();
    QTest::newRow("selection-removal-fails")
        << QString::fromLatin1(CalendarSelectedCacheKey);
    QTest::newRow("global-images-removal-fails")
        << QString::fromLatin1(CalendarLegacyImagesCacheKey);
}

void PersonDQueryTest::calendarMalformedEntriesCleanLegacyState()
{
    QFETCH(QString, blockedKey);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString databasePath = dir.filePath(QStringLiteral("query-cache.sqlite"));
    QueryCacheRepository cache(databasePath);
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarEntriesCacheKey),
                       QStringLiteral("{not-json")), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarSelectedCacheKey),
                       QStringLiteral(R"({"title":"2025-2026"})")),
             qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarLegacyImagesCacheKey),
                       calendarImagesPayload({QStringLiteral("https://example.invalid/legacy.png")})),
             qPrintable(cache.lastError()));
    if (!blockedKey.isNull()) {
        QVERIFY(blockCacheDelete(databasePath, blockedKey));
    }

    AcademicCalendarViewModel model(&cache);
    QSignalSpy toastSpy(&model, &AcademicCalendarViewModel::toastRequested);
    model.readCache();

    QVERIFY(model.entries().isEmpty());
    QCOMPARE(model.selectedIndex(), -1);
    QVERIFY(model.imageUrls().isEmpty());
    QVERIFY(!model.hasCache());

    QueryCacheEntry remaining;
    QVERIFY(!cache.get(QString::fromLatin1(CalendarEntriesCacheKey), &remaining));
    const QStringList legacyKeys{
        QString::fromLatin1(CalendarSelectedCacheKey),
        QString::fromLatin1(CalendarLegacyImagesCacheKey)
    };
    for (const QString &key : legacyKeys) {
        QCOMPARE(cache.get(key, &remaining), key == blockedKey);
    }
    QCOMPARE(toastSpy.count(), blockedKey.isNull() ? 0 : 1);
    if (!blockedKey.isNull()) {
        QCOMPARE(toastSpy.first().first().toString(),
                 QStringLiteral("移除损坏的校历缓存失败，请重试。"));
    }
}

void PersonDQueryTest::calendarKeepsValidEmptyImageCache()
{
    const AcademicCalendarEntry entry{
        QStringLiteral("2025-2026"), QStringLiteral("info/1101/2222.htm")};
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarEntriesCacheKey),
                       calendarEntriesPayload({entry})), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarSelectedCacheKey),
                       calendarSelectedPayload(entry)), qPrintable(cache.lastError()));
    const QString imagesKey = calendarImagesCacheKey(entry.path);
    QVERIFY2(cache.put(imagesKey, QStringLiteral("[]")), qPrintable(cache.lastError()));

    AcademicCalendarViewModel model(&cache);
    model.readCache();

    QVERIFY(model.imageUrls().isEmpty());
    QueryCacheEntry cachedImages;
    QVERIFY2(cache.get(imagesKey, &cachedImages), qPrintable(cache.lastError()));
    QCOMPARE(cachedImages.payloadJson, QStringLiteral("[]"));
}

void PersonDQueryTest::calendarMigratesLegacySelectionAndImages()
{
    const AcademicCalendarEntry first{
        QStringLiteral("2024-2025"), QStringLiteral("info/1101/1111.htm")};
    const AcademicCalendarEntry second{
        QStringLiteral("2025-2026"), QStringLiteral("info/1101/2222.htm")};
    const QStringList legacyImages{QStringLiteral("https://example.invalid/legacy.png")};

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarEntriesCacheKey),
                       calendarEntriesPayload({first, second})), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarSelectedCacheKey),
                       QStringLiteral(R"({"title":"2025-2026"})")), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarLegacyImagesCacheKey),
                       calendarImagesPayload(legacyImages)), qPrintable(cache.lastError()));

    AcademicCalendarViewModel model(&cache);
    QSignalSpy toastSpy(&model, &AcademicCalendarViewModel::toastRequested);
    model.readCache();

    QCOMPARE(model.selectedIndex(), 1);
    QCOMPARE(model.imageUrls(), legacyImages);
    QCOMPARE(toastSpy.count(), 0);

    QueryCacheEntry migratedSelection;
    QVERIFY2(cache.get(QString::fromLatin1(CalendarSelectedCacheKey), &migratedSelection),
             qPrintable(cache.lastError()));
    const QJsonObject selection = QJsonDocument::fromJson(
        migratedSelection.payloadJson.toUtf8()).object();
    QCOMPARE(selection.value(QStringLiteral("title")).toString(), second.title);
    QCOMPARE(selection.value(QStringLiteral("path")).toString(), second.path);

    QueryCacheEntry migratedImages;
    QVERIFY2(cache.get(calendarImagesCacheKey(second.path), &migratedImages),
             qPrintable(cache.lastError()));
    QCOMPARE(migratedImages.payloadJson, calendarImagesPayload(legacyImages));
    QVERIFY(!cache.get(QString::fromLatin1(CalendarLegacyImagesCacheKey), &migratedImages));
    QVERIFY(cache.lastError().isEmpty());
}

void PersonDQueryTest::calendarDoesNotMigrateLegacyImagesToDuplicateTitle()
{
    const AcademicCalendarEntry first{
        QStringLiteral("2025-2026"), QStringLiteral("info/1101/1111.htm")};
    const AcademicCalendarEntry second{
        first.title, QStringLiteral("info/1101/2222.htm")};
    const QString stalePath = QStringLiteral("info/1101/obsolete.htm");
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarEntriesCacheKey),
                       calendarEntriesPayload({first, second})), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarSelectedCacheKey),
                       QString::fromUtf8(QJsonDocument(QJsonObject{
                           {QStringLiteral("path"), stalePath},
                           {QStringLiteral("title"), first.title}
                       }).toJson(QJsonDocument::Compact))), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarLegacyImagesCacheKey),
                       calendarImagesPayload({QStringLiteral("https://example.invalid/legacy.png")})),
             qPrintable(cache.lastError()));

    AcademicCalendarViewModel model(&cache);
    model.readCache();

    QCOMPARE(model.selectedIndex(), 0);
    QVERIFY(model.imageUrls().isEmpty());
    QueryCacheEntry cacheEntry;
    QVERIFY(!cache.get(QString::fromLatin1(CalendarLegacyImagesCacheKey), &cacheEntry));
    QVERIFY(!cache.get(calendarImagesCacheKey(first.path), &cacheEntry));
    QVERIFY(!cache.get(calendarImagesCacheKey(second.path), &cacheEntry));
}

void PersonDQueryTest::calendarDropsUnmatchedLegacySelectionAndImages()
{
    const AcademicCalendarEntry entry{
        QStringLiteral("2025-2026"), QStringLiteral("info/1101/2222.htm")};
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarEntriesCacheKey),
                       calendarEntriesPayload({entry})), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarSelectedCacheKey),
                       QStringLiteral(R"({"title":"missing"})")), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarLegacyImagesCacheKey),
                       calendarImagesPayload({QStringLiteral("https://example.invalid/legacy.png")})),
             qPrintable(cache.lastError()));

    AcademicCalendarViewModel model(&cache);
    model.readCache();

    QCOMPARE(model.selectedIndex(), 0);
    QVERIFY(model.imageUrls().isEmpty());
    QueryCacheEntry removed;
    QVERIFY(!cache.get(QString::fromLatin1(CalendarSelectedCacheKey), &removed));
    QVERIFY(!cache.get(QString::fromLatin1(CalendarLegacyImagesCacheKey), &removed));
    QVERIFY(!cache.get(calendarImagesCacheKey(entry.path), &removed));
}

void PersonDQueryTest::calendarCachePutFailuresKeepNetworkData()
{
    const AcademicCalendarEntry entry{
        QStringLiteral("2025-2026"), QStringLiteral("info/1101/2222.htm")};
    const QStringList images{QStringLiteral("https://example.invalid/network.png")};
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString databasePath = dir.filePath(QStringLiteral("query-cache.sqlite"));
    QueryCacheRepository cache(databasePath);
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));
    QVERIFY(dropCacheTable(databasePath));

    StaticNetworkAccessManager network;
    network.responses = {
        {QByteArray{}, QByteArrayLiteral("text/html; charset=utf-8"), 200,
         QNetworkReply::NoError, true},
        {QByteArray{}, QByteArrayLiteral("text/html; charset=utf-8"), 200,
         QNetworkReply::NoError, true}
    };
    AcademicCalendarService service(&network);
    AcademicCalendarViewModel model(&cache, &service, nullptr);
    QSignalSpy toastSpy(&model, &AcademicCalendarViewModel::toastRequested);

    model.applyEntries({entry}, true);

    QCOMPARE(model.entries().size(), 1);
    QCOMPARE(model.selectedIndex(), 0);
    QVERIFY(!model.hasCache());
    QCOMPARE(toastSpy.count(), 1);
    QCOMPARE(toastSpy.first().first().toString(),
             QStringLiteral("校历数据已更新，但缓存写入失败。"));

    model.applyDetail({entry, images}, true);

    QCOMPARE(model.imageUrls(), images);
    QVERIFY(model.lastUpdated().isValid());
    QVERIFY(!model.hasCache());
    QCOMPARE(toastSpy.count(), 2);
    QCOMPARE(toastSpy.last().first().toString(),
             QStringLiteral("校历数据已更新，但缓存写入失败。"));

    model.reloadSelected();
    QCOMPARE(model.state(), QueryState::Refreshing);
    QVERIFY(model.loading());
    QCOMPARE(model.imageUrls(), images);
    QCOMPARE(network.requests.size(), 2);
}

void PersonDQueryTest::calendarMalformedCacheRemovalFailureIsSafe()
{
    const AcademicCalendarEntry entry{
        QStringLiteral("2025-2026"), QStringLiteral("info/1101/2222.htm")};
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString databasePath = dir.filePath(QStringLiteral("query-cache.sqlite"));
    QueryCacheRepository cache(databasePath);
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarEntriesCacheKey),
                       calendarEntriesPayload({entry})), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarSelectedCacheKey),
                       calendarSelectedPayload(entry)), qPrintable(cache.lastError()));
    const QString imagesKey = calendarImagesCacheKey(entry.path);
    const QString malformed = QStringLiteral("{not-json");
    QVERIFY2(cache.put(imagesKey, malformed), qPrintable(cache.lastError()));
    QVERIFY(blockCacheDelete(databasePath, imagesKey));

    AcademicCalendarViewModel model(&cache);
    QSignalSpy toastSpy(&model, &AcademicCalendarViewModel::toastRequested);
    model.readCache();

    QCOMPARE(model.entries().size(), 1);
    QCOMPARE(model.selectedIndex(), 0);
    QVERIFY(model.imageUrls().isEmpty());
    QVERIFY(model.hasCache());
    QCOMPARE(toastSpy.count(), 1);
    const QString warning = toastSpy.first().first().toString();
    QCOMPARE(warning, QStringLiteral("移除损坏的校历缓存失败，请重试。"));
    QVERIFY(!warning.contains(malformed));
    QVERIFY(!warning.contains(QStringLiteral("blocked cache delete")));
    QueryCacheEntry remaining;
    QVERIFY(cache.get(imagesKey, &remaining));
}

void PersonDQueryTest::calendarClearRemovesKnownKeysAndResetsState()
{
    const AcademicCalendarEntry first{
        QStringLiteral("2024-2025"), QStringLiteral("info/1101/1111.htm")};
    const AcademicCalendarEntry second{
        QStringLiteral("2025-2026"), QStringLiteral("info/1101/2222.htm")};
    const QStringList secondImages{QStringLiteral("https://example.invalid/second.png")};
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));
    const QDateTime cachedAt = QDateTime::fromString(
        QStringLiteral("2026-01-02T03:04:05.000Z"), Qt::ISODateWithMs);
    QVERIFY2(cache.put(QString::fromLatin1(CalendarEntriesCacheKey),
                       calendarEntriesPayload({first, second}), cachedAt), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarSelectedCacheKey),
                       calendarSelectedPayload(second), cachedAt), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(calendarImagesCacheKey(first.path),
                       calendarImagesPayload({QStringLiteral("https://example.invalid/first.png")}),
                       cachedAt), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(calendarImagesCacheKey(second.path),
                       calendarImagesPayload(secondImages), cachedAt), qPrintable(cache.lastError()));

    StaticNetworkAccessManager network;
    network.responses.append({
        QStringLiteral("<img src=\"/__local/new/calendar.png\">").toUtf8(),
        QByteArrayLiteral("text/html; charset=utf-8"), 200, QNetworkReply::NoError, true});
    AcademicCalendarService service(&network);
    AcademicCalendarViewModel model(&cache, &service, nullptr);
    model.readCache();
    QVERIFY2(cache.put(QString::fromLatin1(CalendarLegacyImagesCacheKey),
                       QStringLiteral("[]")), qPrintable(cache.lastError()));
    model.setState(QueryState::Loaded);
    model.reloadSelected();
    QCOMPARE(network.replies.size(), 1);
    StaticNetworkReply *pending = network.replies.first();
    model.setError(QStringLiteral("旧错误"));
    QSignalSpy toastSpy(&model, &AcademicCalendarViewModel::toastRequested);

    model.clearCache();

    QCOMPARE(model.state(), QueryState::Idle);
    QVERIFY(model.errorMessage().isEmpty());
    QVERIFY(!model.lastUpdated().isValid());
    QVERIFY(!model.hasCache());
    QVERIFY(model.entries().isEmpty());
    QCOMPARE(model.selectedIndex(), -1);
    QVERIFY(model.imageUrls().isEmpty());
    QCOMPARE(toastSpy.count(), 0);

    QueryCacheEntry removed;
    const QStringList keys{
        QString::fromLatin1(CalendarEntriesCacheKey),
        QString::fromLatin1(CalendarSelectedCacheKey),
        QString::fromLatin1(CalendarLegacyImagesCacheKey),
        calendarImagesCacheKey(first.path),
        calendarImagesCacheKey(second.path)
    };
    for (const QString &key : keys) {
        QVERIFY2(!cache.get(key, &removed), qPrintable(key));
        QVERIFY(cache.lastError().isEmpty());
    }

    pending->complete();
    QCOMPARE(model.state(), QueryState::Idle);
    QVERIFY(model.entries().isEmpty());
    QVERIFY(model.imageUrls().isEmpty());
    QCOMPARE(toastSpy.count(), 0);
}

void PersonDQueryTest::calendarClearRemovesStaleSelectedPathImages()
{
    const AcademicCalendarEntry current{
        QStringLiteral("2025-2026"), QStringLiteral("info/1101/current.htm")};
    const QString stalePath = QStringLiteral("info/1101/obsolete.htm");
    const QString staleImagesKey = calendarImagesCacheKey(stalePath);
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarEntriesCacheKey),
                       calendarEntriesPayload({current})), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarSelectedCacheKey),
                       QString::fromUtf8(QJsonDocument(QJsonObject{
                           {QStringLiteral("path"), stalePath},
                           {QStringLiteral("title"), current.title}
                       }).toJson(QJsonDocument::Compact))), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(staleImagesKey,
                       calendarImagesPayload({QStringLiteral("https://example.invalid/stale.png")})),
             qPrintable(cache.lastError()));

    AcademicCalendarViewModel model(&cache);
    model.readCache();
    QCOMPARE(model.selectedIndex(), 0);
    QVERIFY(model.imageUrls().isEmpty());

    model.clearCache();

    QCOMPARE(model.state(), QueryState::Idle);
    QVERIFY(!model.hasCache());
    QueryCacheEntry removed;
    QVERIFY(!cache.get(staleImagesKey, &removed));
}

void PersonDQueryTest::calendarClearPartialFailureKeepsDataAndCanRetry()
{
    const AcademicCalendarEntry first{
        QStringLiteral("2024-2025"), QStringLiteral("info/1101/1111.htm")};
    const AcademicCalendarEntry second{
        QStringLiteral("2025-2026"), QStringLiteral("info/1101/2222.htm")};
    const QStringList secondImages{QStringLiteral("https://example.invalid/second.png")};
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString databasePath = dir.filePath(QStringLiteral("query-cache.sqlite"));
    QueryCacheRepository cache(databasePath);
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarEntriesCacheKey),
                       calendarEntriesPayload({first, second})), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarSelectedCacheKey),
                       calendarSelectedPayload(second)), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(calendarImagesCacheKey(first.path),
                       calendarImagesPayload({QStringLiteral("https://example.invalid/first.png")})),
             qPrintable(cache.lastError()));
    const QString failedKey = calendarImagesCacheKey(second.path);
    QVERIFY2(cache.put(failedKey, calendarImagesPayload(secondImages)), qPrintable(cache.lastError()));

    AcademicCalendarViewModel model(&cache);
    model.readCache();
    QVERIFY2(cache.put(QString::fromLatin1(CalendarLegacyImagesCacheKey),
                       QStringLiteral("[]")), qPrintable(cache.lastError()));
    model.setState(QueryState::Loaded);
    model.setError(QStringLiteral("旧错误"));
    const QVariantList preservedEntries = model.entries();
    const QStringList preservedImages = model.imageUrls();
    const QDateTime preservedUpdated = model.lastUpdated();
    const int preservedIndex = model.selectedIndex();
    QVERIFY(blockCacheDelete(databasePath, failedKey));
    QSignalSpy toastSpy(&model, &AcademicCalendarViewModel::toastRequested);

    model.clearCache();

    QCOMPARE(model.state(), QueryState::Loaded);
    QCOMPARE(model.errorMessage(), QStringLiteral("旧错误"));
    QCOMPARE(model.entries(), preservedEntries);
    QCOMPARE(model.imageUrls(), preservedImages);
    QCOMPARE(model.lastUpdated(), preservedUpdated);
    QCOMPARE(model.selectedIndex(), preservedIndex);
    QVERIFY(model.hasCache());
    QCOMPARE(toastSpy.count(), 1);
    QCOMPARE(toastSpy.first().first().toString(),
             QStringLiteral("清除校历缓存失败，请重试。"));

    QueryCacheEntry cacheEntry;
    QVERIFY(!cache.get(QString::fromLatin1(CalendarEntriesCacheKey), &cacheEntry));
    QVERIFY(!cache.get(QString::fromLatin1(CalendarSelectedCacheKey), &cacheEntry));
    QVERIFY(!cache.get(QString::fromLatin1(CalendarLegacyImagesCacheKey), &cacheEntry));
    QVERIFY(!cache.get(calendarImagesCacheKey(first.path), &cacheEntry));
    QVERIFY(cache.get(failedKey, &cacheEntry));

    QVERIFY(unblockCacheDelete(databasePath));
    model.clearCache();

    QCOMPARE(model.state(), QueryState::Idle);
    QVERIFY(model.errorMessage().isEmpty());
    QVERIFY(!model.lastUpdated().isValid());
    QVERIFY(!model.hasCache());
    QVERIFY(model.entries().isEmpty());
    QCOMPARE(model.selectedIndex(), -1);
    QVERIFY(model.imageUrls().isEmpty());
    QCOMPARE(toastSpy.count(), 1);
    QVERIFY(!cache.get(failedKey, &cacheEntry));
}

void PersonDQueryTest::calendarCachedRequestsRefreshAndRetainOnFailure_data()
{
    QTest::addColumn<bool>("listRequest");

    QTest::newRow("list") << true;
    QTest::newRow("detail") << false;
}

void PersonDQueryTest::calendarCachedRequestsRefreshAndRetainOnFailure()
{
    QFETCH(bool, listRequest);

    const AcademicCalendarEntry entry{
        QStringLiteral("2025-2026"), QStringLiteral("info/1101/2222.htm")};
    const QStringList images{QStringLiteral("https://example.invalid/cached.png")};
    const QDateTime cachedAt = QDateTime::fromString(
        QStringLiteral("2026-01-02T03:04:05.000Z"), Qt::ISODateWithMs);
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarEntriesCacheKey),
                       calendarEntriesPayload({entry}), cachedAt), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarSelectedCacheKey),
                       calendarSelectedPayload(entry), cachedAt), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(calendarImagesCacheKey(entry.path),
                       calendarImagesPayload(images), cachedAt), qPrintable(cache.lastError()));

    StaticNetworkAccessManager network;
    for (int i = 0; i < 2; ++i) {
        network.responses.append({QByteArray{}, QByteArrayLiteral("text/html; charset=utf-8"), 0,
                                  QNetworkReply::HostNotFoundError, true});
    }
    AcademicCalendarService service(&network);
    AcademicCalendarViewModel model(&cache, &service, nullptr);
    model.readCache();
    model.setState(QueryState::Loaded);
    const QVariantList preservedEntries = model.entries();
    const QStringList preservedImages = model.imageUrls();
    const QDateTime preservedUpdated = model.lastUpdated();
    QSignalSpy toastSpy(&model, &AcademicCalendarViewModel::toastRequested);

    if (listRequest) {
        model.reloadList();
        model.reloadList();
    } else {
        model.reloadSelected();
        model.reloadSelected();
    }

    QCOMPARE(model.state(), QueryState::Refreshing);
    QVERIFY(model.loading());
    QCOMPARE(model.entries(), preservedEntries);
    QCOMPARE(model.imageUrls(), preservedImages);
    QCOMPARE(model.lastUpdated(), preservedUpdated);
    QCOMPARE(network.requests.size(), 1);

    network.replies.first()->complete();

    QTRY_COMPARE(model.state(), QueryState::Loaded);
    QCOMPARE(model.entries(), preservedEntries);
    QCOMPARE(model.imageUrls(), preservedImages);
    QCOMPARE(model.lastUpdated(), preservedUpdated);
    QVERIFY(model.errorMessage().isEmpty());
    QCOMPARE(toastSpy.count(), 1);
    QCOMPARE(toastSpy.first().first().toString(),
             QStringLiteral("校历网络请求失败，请检查网络后重试。"));
}

void PersonDQueryTest::calendarEmptyCachesAndVisibleEntriesRefreshSafely_data()
{
    QTest::addColumn<bool>("listRequest");
    QTest::addColumn<bool>("storeEntry");
    QTest::addColumn<bool>("storeEmptyImages");

    QTest::newRow("empty-list-cache") << true << false << false;
    QTest::newRow("empty-image-cache") << false << true << true;
    QTest::newRow("visible-entries-without-image-cache") << false << true << false;
}

void PersonDQueryTest::calendarEmptyCachesAndVisibleEntriesRefreshSafely()
{
    QFETCH(bool, listRequest);
    QFETCH(bool, storeEntry);
    QFETCH(bool, storeEmptyImages);

    const AcademicCalendarEntry entry{
        QStringLiteral("2025-2026"), QStringLiteral("info/1101/2222.htm")};
    const QDateTime cachedAt = QDateTime::fromString(
        QStringLiteral("2026-01-02T03:04:05.000Z"), Qt::ISODateWithMs);
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(CalendarEntriesCacheKey),
                       calendarEntriesPayload(storeEntry
                                                  ? QList<AcademicCalendarEntry>{entry}
                                                  : QList<AcademicCalendarEntry>{}),
                       cachedAt), qPrintable(cache.lastError()));
    if (storeEntry) {
        QVERIFY2(cache.put(QString::fromLatin1(CalendarSelectedCacheKey),
                           calendarSelectedPayload(entry), cachedAt), qPrintable(cache.lastError()));
    }
    if (storeEmptyImages) {
        QVERIFY2(cache.put(calendarImagesCacheKey(entry.path), QStringLiteral("[]"), cachedAt),
                 qPrintable(cache.lastError()));
    }

    StaticNetworkAccessManager network;
    network.responses.append({QByteArray{}, QByteArrayLiteral("text/html; charset=utf-8"), 0,
                              QNetworkReply::HostNotFoundError, true});
    AcademicCalendarService service(&network);
    AcademicCalendarViewModel model(&cache, &service, nullptr);
    model.readCache();
    model.setState(QueryState::Empty);
    const QVariantList preservedEntries = model.entries();
    const QDateTime preservedUpdated = model.lastUpdated();
    QSignalSpy toastSpy(&model, &AcademicCalendarViewModel::toastRequested);

    if (listRequest) {
        model.reloadList();
    } else {
        model.reloadSelected();
    }

    QCOMPARE(model.state(), QueryState::Refreshing);
    QVERIFY(model.loading());
    QCOMPARE(model.entries(), preservedEntries);
    QVERIFY(model.imageUrls().isEmpty());
    QCOMPARE(model.lastUpdated(), preservedUpdated);
    QCOMPARE(network.requests.size(), 1);

    network.replies.first()->complete();

    QTRY_COMPARE(model.state(), QueryState::Empty);
    QCOMPARE(model.entries(), preservedEntries);
    QVERIFY(model.imageUrls().isEmpty());
    QCOMPARE(model.lastUpdated(), preservedUpdated);
    QVERIFY(model.errorMessage().isEmpty());
    QCOMPARE(toastSpy.count(), 1);
    QCOMPARE(toastSpy.first().first().toString(),
             QStringLiteral("校历网络请求失败，请检查网络后重试。"));
}

void PersonDQueryTest::calendarLoadWithoutCacheStartsListRequest()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));
    StaticNetworkAccessManager network;
    network.responses.append({QByteArray{}, QByteArrayLiteral("text/html; charset=utf-8"),
                              200, QNetworkReply::NoError, true});
    AcademicCalendarService service(&network);
    AcademicCalendarViewModel model(&cache, &service, nullptr);

    model.load();

    QCOMPARE(model.state(), QueryState::Loading);
    QVERIFY(model.loading());
    QCOMPARE(network.requests.size(), 1);
}

void PersonDQueryTest::calendarImageViewerShowsLoadingAndFailureFallback()
{
    const QString viewerPath = QFINDTESTDATA(
        "../qml/pages/calendar/CalendarImageViewer.qml");
    QVERIFY2(!viewerPath.isEmpty(), "CalendarImageViewer.qml fixture not found");
    QFile viewerFile(viewerPath);
    QVERIFY(viewerFile.open(QIODevice::ReadOnly));
    const QString source = QString::fromUtf8(viewerFile.readAll());

    QVERIFY(source.contains(QStringLiteral("BusyIndicator")));
    QVERIFY(source.contains(QStringLiteral("Image.Loading")));
    QVERIFY(source.contains(QStringLiteral("Image.Error")));
    QVERIFY(source.contains(QStringLiteral("校历图片加载失败，请检查网络后重试。")));
    QVERIFY(source.contains(QStringLiteral("standardButtons: Dialog.Close")));
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

void PersonDQueryTest::cachedExamRefreshUsesRefreshingAndKeepsContent()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));

    const QDateTime cachedAt = QDateTime::fromString(
        QStringLiteral("2025-01-02T03:04:05.000Z"), Qt::ISODateWithMs);
    ExamPlanItem cachedItem;
    cachedItem.courseName = QStringLiteral("缓存考试");
    cachedItem.date = QStringLiteral("2026-01-08");
    cachedItem.timeRange = QStringLiteral("09:00-11:00");
    const QString payload = QString::fromUtf8(
        QJsonDocument(examPlanItemsToJson({cachedItem})).toJson(QJsonDocument::Compact));
    QVERIFY2(cache.put(QString::fromLatin1(ExamPlanCacheKey), payload, cachedAt),
             qPrintable(cache.lastError()));

    DeferredZhjwQueryService service;
    service.loggedInValue = false;
    ExamPlanViewModel model(&cache, &service);
    model.load();
    QCOMPARE(model.state(), QueryState::Loaded);
    const QVariantList cachedExams = model.exams();
    QCOMPARE(model.lastUpdated(), cachedAt);
    QVERIFY(model.hasCache());

    service.setLoggedIn(true);
    QSignalSpy toastSpy(&model, &ExamPlanViewModel::toastRequested);
    model.refresh();

    QCOMPARE(model.state(), QueryState::Refreshing);
    QVERIFY(model.loading());
    QCOMPARE(model.exams(), cachedExams);
    QCOMPARE(model.lastUpdated(), cachedAt);
    QVERIFY(model.hasCache());
    QCOMPARE(service.examFetchCount, 1);

    model.refresh();
    QCOMPARE(service.examFetchCount, 1);

    const QString errorMessage = QStringLiteral("考试会话已过期，请重新登录");
    service.completeExam({}, ApiError{
        ApiErrorType::SessionExpired, errorMessage, 401});

    QCOMPARE(model.state(), QueryState::Loaded);
    QCOMPARE(model.exams(), cachedExams);
    QCOMPARE(model.lastUpdated(), cachedAt);
    QVERIFY(model.hasCache());
    QCOMPARE(model.errorMessage(), errorMessage);
    QCOMPARE(toastSpy.count(), 1);
    QCOMPARE(toastSpy.first().first().toString(), errorMessage);

    QueryCacheEntry preservedEntry;
    QVERIFY2(cache.get(QString::fromLatin1(ExamPlanCacheKey), &preservedEntry),
             qPrintable(cache.lastError()));
    QCOMPARE(preservedEntry.payloadJson, payload);
    QCOMPARE(preservedEntry.updatedAt, cachedAt);
}

void PersonDQueryTest::cachedSchemeRefreshUsesRefreshingAndKeepsContent()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));

    const QDateTime cachedAt = QDateTime::fromString(
        QStringLiteral("2025-02-03T04:05:06.000Z"), Qt::ISODateWithMs);
    const QString schemePayload = QString::fromUtf8(
        QJsonDocument(sampleSchemeRoot()).toJson(QJsonDocument::Compact));
    const QString passingPayload = QString::fromUtf8(
        QJsonDocument(QJsonObject{{QStringLiteral("lnList"), QJsonArray{}}})
            .toJson(QJsonDocument::Compact));
    QVERIFY2(cache.put(QString::fromLatin1(SchemeCacheKey), schemePayload, cachedAt),
             qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(PassingCacheKey), passingPayload, cachedAt),
             qPrintable(cache.lastError()));

    DeferredZhjwQueryService service;
    GradesViewModel model(&cache, &service);
    model.load();
    QCOMPARE(model.schemeState(), QueryState::Loaded);
    const QVariantList cachedGroups = model.schemeGroups();
    const QVariantMap cachedSummary = model.schemeSummary();
    QCOMPARE(model.schemeLastUpdated(), cachedAt);

    QSignalSpy toastSpy(&model, &GradesViewModel::toastRequested);
    model.refreshSchemeScores();

    QCOMPARE(model.schemeState(), QueryState::Refreshing);
    QCOMPARE(model.schemeGroups(), cachedGroups);
    QCOMPARE(model.schemeSummary(), cachedSummary);
    QCOMPARE(model.schemeLastUpdated(), cachedAt);
    QCOMPARE(service.schemeFetchCount, 1);

    model.refreshSchemeScores();
    QCOMPARE(service.schemeFetchCount, 1);

    const QString errorMessage = QStringLiteral("方案成绩网络暂不可用");
    service.completeScheme({}, ApiError{
        ApiErrorType::Network, errorMessage, 503});

    QCOMPARE(model.schemeState(), QueryState::Loaded);
    QCOMPARE(model.schemeGroups(), cachedGroups);
    QCOMPARE(model.schemeSummary(), cachedSummary);
    QCOMPARE(model.schemeLastUpdated(), cachedAt);
    QCOMPARE(model.schemeErrorMessage(), errorMessage);
    QCOMPARE(toastSpy.count(), 1);
    QCOMPARE(toastSpy.first().first().toString(), errorMessage);

    QueryCacheEntry preservedEntry;
    QVERIFY2(cache.get(QString::fromLatin1(SchemeCacheKey), &preservedEntry),
             qPrintable(cache.lastError()));
    QCOMPARE(preservedEntry.payloadJson, schemePayload);
    QCOMPARE(preservedEntry.updatedAt, cachedAt);
}

void PersonDQueryTest::cachedPassingRefreshUsesRefreshingAndKeepsContent()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));

    const QDateTime cachedAt = QDateTime::fromString(
        QStringLiteral("2025-03-04T05:06:07.000Z"), Qt::ISODateWithMs);
    const QString schemePayload = QString::fromUtf8(
        QJsonDocument(QJsonObject{{QStringLiteral("lnList"), QJsonArray{}}})
            .toJson(QJsonDocument::Compact));
    const QString passingPayload = QString::fromUtf8(
        QJsonDocument(samplePassingRoot()).toJson(QJsonDocument::Compact));
    QVERIFY2(cache.put(QString::fromLatin1(SchemeCacheKey), schemePayload, cachedAt),
             qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(PassingCacheKey), passingPayload, cachedAt),
             qPrintable(cache.lastError()));

    DeferredZhjwQueryService service;
    GradesViewModel model(&cache, &service);
    model.load();
    QCOMPARE(model.passingState(), QueryState::Loaded);
    const QVariantList cachedGroups = model.passingGroups();
    const QVariantMap cachedSummary = model.passingSummary();
    QCOMPARE(model.passingLastUpdated(), cachedAt);

    QSignalSpy toastSpy(&model, &GradesViewModel::toastRequested);
    model.refreshPassingScores();

    QCOMPARE(model.passingState(), QueryState::Refreshing);
    QCOMPARE(model.passingGroups(), cachedGroups);
    QCOMPARE(model.passingSummary(), cachedSummary);
    QCOMPARE(model.passingLastUpdated(), cachedAt);
    QCOMPARE(service.passingFetchCount, 1);

    model.refreshPassingScores();
    QCOMPARE(service.passingFetchCount, 1);

    const QString errorMessage = QStringLiteral("及格成绩登录已失效");
    service.completePassing({}, ApiError{
        ApiErrorType::Unauthenticated, errorMessage, 401});

    QCOMPARE(model.passingState(), QueryState::Loaded);
    QCOMPARE(model.passingGroups(), cachedGroups);
    QCOMPARE(model.passingSummary(), cachedSummary);
    QCOMPARE(model.passingLastUpdated(), cachedAt);
    QCOMPARE(model.passingErrorMessage(), errorMessage);
    QCOMPARE(toastSpy.count(), 1);
    QCOMPARE(toastSpy.first().first().toString(), errorMessage);

    QueryCacheEntry preservedEntry;
    QVERIFY2(cache.get(QString::fromLatin1(PassingCacheKey), &preservedEntry),
             qPrintable(cache.lastError()));
    QCOMPARE(preservedEntry.payloadJson, passingPayload);
    QCOMPARE(preservedEntry.updatedAt, cachedAt);
}

void PersonDQueryTest::emptyCachedSchemeRefreshFailureReturnsEmpty()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));

    const QDateTime cachedAt = QDateTime::fromString(
        QStringLiteral("2025-04-05T06:07:08.000Z"), Qt::ISODateWithMs);
    const QString emptyPayload = QString::fromUtf8(
        QJsonDocument(QJsonObject{{QStringLiteral("lnList"), QJsonArray{}}})
            .toJson(QJsonDocument::Compact));
    QVERIFY2(cache.put(QString::fromLatin1(SchemeCacheKey), emptyPayload, cachedAt),
             qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(PassingCacheKey), emptyPayload, cachedAt),
             qPrintable(cache.lastError()));

    DeferredZhjwQueryService service;
    GradesViewModel model(&cache, &service);
    model.load();
    QCOMPARE(model.schemeState(), QueryState::Empty);
    QVERIFY(model.schemeGroups().isEmpty());
    const QVariantMap cachedSummary = model.schemeSummary();
    QCOMPARE(model.schemeLastUpdated(), cachedAt);

    QSignalSpy toastSpy(&model, &GradesViewModel::toastRequested);
    model.refreshSchemeScores();
    QCOMPARE(model.schemeState(), QueryState::Refreshing);
    QVERIFY(model.schemeGroups().isEmpty());
    QCOMPARE(model.schemeSummary(), cachedSummary);
    QCOMPARE(model.schemeLastUpdated(), cachedAt);
    QCOMPARE(service.schemeFetchCount, 1);

    model.refreshSchemeScores();
    QCOMPARE(service.schemeFetchCount, 1);

    const QString errorMessage = QStringLiteral("空方案成绩刷新失败");
    service.completeScheme({}, ApiError{
        ApiErrorType::Network, errorMessage, 503});

    QCOMPARE(model.schemeState(), QueryState::Empty);
    QVERIFY(model.schemeGroups().isEmpty());
    QCOMPARE(model.schemeSummary(), cachedSummary);
    QCOMPARE(model.schemeLastUpdated(), cachedAt);
    QCOMPARE(model.schemeErrorMessage(), errorMessage);
    QCOMPARE(toastSpy.count(), 1);
    QCOMPARE(toastSpy.first().first().toString(), errorMessage);

    QueryCacheEntry preservedEntry;
    QVERIFY2(cache.get(QString::fromLatin1(SchemeCacheKey), &preservedEntry),
             qPrintable(cache.lastError()));
    QCOMPARE(preservedEntry.payloadJson, emptyPayload);
    QCOMPARE(preservedEntry.updatedAt, cachedAt);
}

void PersonDQueryTest::emptyCachedGradeRefreshWhileLoggedOutStaysEmpty_data()
{
    QTest::addColumn<bool>("scheme");

    QTest::newRow("scheme") << true;
    QTest::newRow("passing") << false;
}

void PersonDQueryTest::emptyCachedGradeRefreshWhileLoggedOutStaysEmpty()
{
    QFETCH(bool, scheme);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));

    const QDateTime cachedAt = QDateTime::fromString(
        QStringLiteral("2025-05-06T07:08:09.000Z"), Qt::ISODateWithMs);
    const QString emptyPayload = QString::fromUtf8(
        QJsonDocument(QJsonObject{{QStringLiteral("lnList"), QJsonArray{}}})
            .toJson(QJsonDocument::Compact));
    QVERIFY2(cache.put(QString::fromLatin1(SchemeCacheKey), emptyPayload, cachedAt),
             qPrintable(cache.lastError()));
    QVERIFY2(cache.put(QString::fromLatin1(PassingCacheKey), emptyPayload, cachedAt),
             qPrintable(cache.lastError()));

    DeferredZhjwQueryService service;
    service.loggedInValue = false;
    GradesViewModel model(&cache, &service);
    model.load();
    QCOMPARE(model.schemeState(), QueryState::Empty);
    QCOMPARE(model.passingState(), QueryState::Empty);

    QSignalSpy toastSpy(&model, &GradesViewModel::toastRequested);
    if (scheme) {
        model.refreshSchemeScores();
        QCOMPARE(model.schemeState(), QueryState::Empty);
        QCOMPARE(model.schemeLastUpdated(), cachedAt);
        QCOMPARE(service.schemeFetchCount, 0);
    } else {
        model.refreshPassingScores();
        QCOMPARE(model.passingState(), QueryState::Empty);
        QCOMPARE(model.passingLastUpdated(), cachedAt);
        QCOMPARE(service.passingFetchCount, 0);
    }
    QCOMPARE(toastSpy.count(), 1);
}

void PersonDQueryTest::noCacheRefreshFailureStates_data()
{
    QTest::addColumn<QString>("entryPoint");
    QTest::addColumn<int>("errorType");

    for (const QString &entryPoint : {
             QStringLiteral("exam"),
             QStringLiteral("scheme"),
             QStringLiteral("passing")}) {
        QTest::newRow(qPrintable(entryPoint + QStringLiteral("-network")))
            << entryPoint << static_cast<int>(ApiErrorType::Network);
        QTest::newRow(qPrintable(entryPoint + QStringLiteral("-session-expired")))
            << entryPoint << static_cast<int>(ApiErrorType::SessionExpired);
    }
}

void PersonDQueryTest::noCacheRefreshFailureStates()
{
    QFETCH(QString, entryPoint);
    QFETCH(int, errorType);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QueryCacheRepository cache(dir.filePath(QStringLiteral("query-cache.sqlite")));
    QVERIFY2(cache.open(), qPrintable(cache.lastError()));

    const ApiErrorType type = static_cast<ApiErrorType>(errorType);
    const QueryState expectedState = type == ApiErrorType::SessionExpired
        ? QueryState::LoginRequired
        : QueryState::Error;
    const QString errorMessage = QStringLiteral("安全查询错误：%1").arg(entryPoint);
    const ApiError error{type, errorMessage, type == ApiErrorType::SessionExpired ? 401 : 503};
    DeferredZhjwQueryService service;

    if (entryPoint == QStringLiteral("exam")) {
        ExamPlanViewModel model(&cache, &service);
        QSignalSpy toastSpy(&model, &ExamPlanViewModel::toastRequested);

        model.refresh();
        QCOMPARE(model.state(), QueryState::Loading);
        QCOMPARE(service.examFetchCount, 1);
        model.refresh();
        QCOMPARE(service.examFetchCount, 1);
        service.completeExam({}, error);

        QCOMPARE(model.state(), expectedState);
        QCOMPARE(model.errorMessage(), errorMessage);
        QCOMPARE(model.count(), 0);
        QVERIFY(!model.hasCache());
        QVERIFY(!model.lastUpdated().isValid());
        QCOMPARE(toastSpy.count(), 0);
        return;
    }

    GradesViewModel model(&cache, &service);
    QSignalSpy toastSpy(&model, &GradesViewModel::toastRequested);
    if (entryPoint == QStringLiteral("scheme")) {
        model.refreshSchemeScores();
        QCOMPARE(model.schemeState(), QueryState::Loading);
        QCOMPARE(service.schemeFetchCount, 1);
        model.refreshSchemeScores();
        QCOMPARE(service.schemeFetchCount, 1);
        service.completeScheme({}, error);

        QCOMPARE(model.schemeState(), expectedState);
        QCOMPARE(model.schemeErrorMessage(), errorMessage);
        QVERIFY(model.schemeGroups().isEmpty());
        QVERIFY(!model.schemeLastUpdated().isValid());
    } else {
        model.refreshPassingScores();
        QCOMPARE(model.passingState(), QueryState::Loading);
        QCOMPARE(service.passingFetchCount, 1);
        model.refreshPassingScores();
        QCOMPARE(service.passingFetchCount, 1);
        service.completePassing({}, error);

        QCOMPARE(model.passingState(), expectedState);
        QCOMPARE(model.passingErrorMessage(), errorMessage);
        QVERIFY(model.passingGroups().isEmpty());
        QVERIFY(!model.passingLastUpdated().isValid());
    }
    QCOMPARE(toastSpy.count(), 0);
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
