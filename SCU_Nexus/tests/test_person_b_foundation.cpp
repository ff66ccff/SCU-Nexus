#include <QtTest/QtTest>

#include "src/core/network/CookieJar.h"
#include "src/core/crypto/Sm2Crypto.h"
#include "src/core/logging/AuthLogger.h"
#include <QObject>
#include <QUrl>
#define private public
#include "src/app/AppController.h"
#include "src/services/auth/AuthViewModel.h"
#include "src/services/auth/ZhjwAuthService.h"
#include "src/services/api/ZhjwApiService.h"
#include "src/services/zhjw/ZhjwQueryService.h"
#undef private
#include "src/services/auth/ScuAuthService.h"
#include "src/services/api/ZhjwParsers.h"

#include <QEventLoop>
#include <QFile>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <algorithm>

namespace {

// 可编程的 HTTP 假客户端：记录请求，并按 URL 返回预置响应/错误。
// B 层测试因此不会访问真实校园网，也不需要真实账号。
class FakeCookieHttpClient : public CookieHttpClient
{
public:
    explicit FakeCookieHttpClient(QObject* parent = nullptr)
        : CookieHttpClient(parent)
    {
    }

    void get(const QUrl& url, Callback callback, const Headers& headers = {}) override
    {
        getUrls.append(url.toString());
        getHeaders.append(headers);
        callback(nextResponse(url), nextError(url));
    }

    void post(const QUrl& url, const QByteArray& body, Callback callback, const Headers& headers = {}) override
    {
        postUrls.append(url.toString());
        postBodies.append(body);
        postHeaders.append(headers);
        callback(nextResponse(url), nextError(url));
    }

    void followRedirects(const QUrl& url, Callback callback, const Headers& headers = {}, int maxRedirects = 10) override
    {
        Q_UNUSED(maxRedirects)
        redirectUrls.append(url.toString());
        redirectHeaders.append(headers);
        callback(nextResponse(url), nextError(url));
    }

    QStringList getUrls;
    QStringList postUrls;
    QStringList redirectUrls;
    QList<Headers> getHeaders;
    QList<Headers> postHeaders;
    QList<Headers> redirectHeaders;
    QList<QByteArray> postBodies;
    QMap<QString, HttpResponse> responses;
    QMap<QString, QList<HttpResponse>> responseQueues;
    QMap<QString, ApiError> errors;
    QMap<QString, QList<ApiError>> errorQueues;

private:
    ApiError nextError(const QUrl& url)
    {
        const QString key = url.toString();
        if (errorQueues.contains(key) && !errorQueues[key].isEmpty()) {
            return errorQueues[key].takeFirst();
        }
        if (errors.contains(key)) {
            return errors.value(key);
        }
        return {};
    }

    HttpResponse nextResponse(const QUrl& url)
    {
        const QString key = url.toString();
        if (responseQueues.contains(key) && !responseQueues[key].isEmpty()) {
            return responseQueues[key].takeFirst();
        }
        if (responses.contains(key)) {
            return responses.value(key);
        }
        HttpResponse response;
        response.statusCode = 200;
        response.finalUrl = url;
        response.body = "{}";
        return response;
    }
};

// 只统计 getClient/invalidate，用于验证 API 会话过期后是否恰好重试一次。
class FakeZhjwAuthService : public ZhjwAuthService
{
public:
    explicit FakeZhjwAuthService(FakeCookieHttpClient* client)
        : ZhjwAuthService(nullptr, nullptr)
        , fakeClient(client)
    {
    }

    void getClient(ClientCallback callback) override
    {
        ++getClientCount;
        callback(fakeClient, {});
    }

    void invalidate() override
    {
        ++invalidateCount;
    }

    FakeCookieHttpClient* fakeClient = nullptr;
    int getClientCount = 0;
    int invalidateCount = 0;
};

// AuthViewModel 和 ZhjwAuthService 的测试替身，隔离真实验证码、token 与 SSO 请求。
class FakeScuAuthService : public ScuAuthService
{
public:
    explicit FakeScuAuthService(QObject* parent = nullptr)
        : ScuAuthService(parent)
    {
    }

// 验证 fetchCaptcha 场景的预期行为。
    void fetchCaptcha(std::function<void(const CaptchaResult& result, const ApiError& error)> callback) override
    {
        ApiError error;
        error.type = ApiErrorType::Network;
        error.message = QStringLiteral("captcha down");
        callback({}, error);
    }

    void triggerSessionExpired(const QString& message)
    {
        emit sessionExpired(message);
    }
};

class DeferredSessionClient : public CookieHttpClient
{
public:
    explicit DeferredSessionClient(QObject* parent = nullptr)
        : CookieHttpClient(parent)
    {
    }

    void post(const QUrl& url, const QByteArray& body, Callback callback, const Headers& headers = {}) override
    {
        Q_UNUSED(body)
        Q_UNUSED(headers)
        postUrls.append(url.toString());
        pendingCallbacks.append(std::move(callback));
    }

    void completeNextSessionSave()
    {
        QVERIFY(!pendingCallbacks.isEmpty());
        HttpResponse response;
        response.statusCode = 200;
        response.finalUrl = QUrl(QStringLiteral("https://id.scu.edu.cn/api/bff/v1.2/commons/session/save"));
        response.body = R"({"success":true})";
        auto callback = std::move(pendingCallbacks.first());
        pendingCallbacks.removeFirst();
        callback(response, {});
    }

    QStringList postUrls;
    QList<Callback> pendingCallbacks;
};

class DeferredRedirectClient : public CookieHttpClient
{
public:
    explicit DeferredRedirectClient(QObject* parent = nullptr)
        : CookieHttpClient(parent)
    {
    }

    void followRedirects(const QUrl& url, Callback callback, const Headers& headers = {}, int maxRedirects = 10) override
    {
        Q_UNUSED(headers)
        Q_UNUSED(maxRedirects)
        redirectUrls.append(url.toString());
        pendingRedirects.append(std::move(callback));
    }

    void completeNextRedirect()
    {
        QVERIFY(!pendingRedirects.isEmpty());
        HttpResponse response;
        response.statusCode = 200;
        response.finalUrl = QUrl(QStringLiteral("http://zhjw.scu.edu.cn/"));
        response.body = "ok";
        auto callback = std::move(pendingRedirects.first());
        pendingRedirects.removeFirst();
        callback(response, {});
    }

    QStringList redirectUrls;
    QList<Callback> pendingRedirects;
};

class BoundScuAuthService : public ScuAuthService
{
public:
    explicit BoundScuAuthService(CookieHttpClient* client)
        : ScuAuthService(nullptr)
        , boundClient(client)
    {
        saveTokenForTesting(QStringLiteral("access-token-1"), QDateTime::currentSecsSinceEpoch());
    }

    void bindSession(BindSessionCallback callback) override
    {
        ++bindSessionCalls;
        callback(boundClient, {});
    }

    CookieHttpClient* boundClient = nullptr;
    int bindSessionCalls = 0;
};

}

class PersonBFoundationTest : public QObject
{
    Q_OBJECT

private slots:
    void networkErrorTaxonomyIncludesDomainFailures();
    void cookieJarMatchesExactAndParentDomain();
    void cookieJarRejectsUnrelatedDomain();
    void cookieJarIgnoresSetCookieDomainForSubsystemIsolation();
    void cookieJarParsesCommaSeparatedSetCookie();
    void cookieJarDebugSummaryCountsPairsAndExcludesHostsAndValues();
    void cookieHttpClientReturnsHttpErrorResponsesWithBody();
    void cookieHttpClientReportsRedirectLimit();
    void cookieHttpClientSendsCookiesStoredDuringRedirect();
    void authLoggerRedactsSensitiveValues();
    void authLoggerRedactsIdentityCaptchaAndCookieValues();
    void authLoggerRedactsJsonTokenAndFormPasswordCaseInsensitively();
    void authLoggerCapsRingBuffer();
    void zhjwParsersExtractRequiredValues();
    void zhjwExamPlanParserRecognizesValidWidget();
    void zhjwExamPlanParserRecognizesExplicitEmptyMarkers();
    void zhjwExamPlanParserRejectsUnrelatedHtml();
    void zhjwExamPlanParserRecognizesUnparseableWidget();
    void zhjwParsersReadClassroomIndexAndAvailability();
    void zhjwParsersRejectMalformedClassroomPayloads();
    void zhjwParsersDetectExpiredSessions();
    void authViewModelWritesFreshCaptchaUrlEachTime();
    void authViewModelRejectsLoginBeforeCaptchaLoaded();
    void authViewModelDoesNotEmitLoginFailedForCaptchaFetchErrors();
    void authViewModelForwardsSessionExpired();
    void scuAuthParsesCaptchaDataUrl();
    void scuAuthTokenTtlAndStorageWork();
    void zhjwApiExposesDocumentedEndpoints();
    void zhjwApiRequestsClassroomIndexAndAvailability();
    void sm2CryptoEncryptsToC1C2C3Base64();
    void scuAuthLoginRequestsSm2ThenTokenAndStoresToken();
    void scuAuthLoginParsesRestTokenHttp400JsonError();
    void scuAuthBindSessionCoalescesConcurrentCalls();
    void scuAuthBindSessionRefreshesExpiredTokenWhenSessionSaveSucceeds();
    void zhjwAuthCoalescesConcurrentSsoLogin();
    void zhjwAuthRebindsWhenScuTokenChanges();
    void zhjwAuthRevalidatesScuSessionBeforeCacheHit();
    void appControllerCanShareScuAuthWithZhjwStack();
    void appControllerReflectsRestoredAuthState();
    void zhjwApiUsesBrowserAcceptForExamPlan();
    void zhjwApiReturnsValidExamPlan();
    void zhjwApiReturnsExplicitlyEmptyExamPlan();
    void zhjwApiMapsUnrelatedExamPageToParseFailure();
    void zhjwApiMapsUnparseableExamWidgetToParseFailure();
    void zhjwApiSanitizesExamParseFailureSummary();
    void zhjwApiUsesBrowserAcceptForScoreIndex();
    void zhjwApiRetriesScoreIndexWhenLoginMarkerHidesCallback();
    void zhjwApiRetriesOnceWhenSessionExpired();
    void zhjwApiPreservesNetworkErrorWithoutSessionRetry();
};

// 验证网络错误枚举包含完整的领域错误，并保持追加顺序。
void PersonBFoundationTest::networkErrorTaxonomyIncludesDomainFailures()
{
    QCOMPARE(static_cast<int>(ApiErrorType::ConflictDetected),
             static_cast<int>(ApiErrorType::ParameterInvalid) + 1);
    QCOMPARE(static_cast<int>(ApiErrorType::DataWriteFailed),
             static_cast<int>(ApiErrorType::ConflictDetected) + 1);
    QCOMPARE(static_cast<int>(ApiErrorType::EmptyResult),
             static_cast<int>(ApiErrorType::DataWriteFailed) + 1);
    QCOMPARE(static_cast<int>(ApiErrorType::Unknown),
             static_cast<int>(ApiErrorType::EmptyResult) + 1);
}

// 验证 CookieJar 的域名匹配和解析行为。
void PersonBFoundationTest::cookieJarMatchesExactAndParentDomain()
{
    CookieJar jar;
    jar.storeFromSetCookie(QUrl("https://id.scu.edu.cn/login"),
                           {QByteArray("SESSION=abc; Path=/; HttpOnly")});

    QCOMPARE(jar.cookieHeader(QUrl("https://id.scu.edu.cn/profile")), QString("SESSION=abc"));
    QCOMPARE(jar.cookieHeader(QUrl("https://sub.id.scu.edu.cn/profile")), QString("SESSION=abc"));
}

// 验证 CookieJar 的域名匹配和解析行为。
void PersonBFoundationTest::cookieJarRejectsUnrelatedDomain()
{
    CookieJar jar;
    jar.storeFromSetCookie(QUrl("https://id.scu.edu.cn/login"),
                           {QByteArray("SESSION=abc; Path=/; HttpOnly")});

    QCOMPARE(jar.cookieHeader(QUrl("https://zhjw.scu.edu.cn/")), QString());
}

// 验证 CookieJar 的域名匹配和解析行为。
void PersonBFoundationTest::cookieJarIgnoresSetCookieDomainForSubsystemIsolation()
{
    CookieJar jar;
    jar.storeFromSetCookie(QUrl("https://id.scu.edu.cn/login"),
                           {QByteArray("SESSION=abc; Domain=.scu.edu.cn; Path=/; HttpOnly")});

    QCOMPARE(jar.cookieHeader(QUrl("https://id.scu.edu.cn/profile")), QString("SESSION=abc"));
    QCOMPARE(jar.cookieHeader(QUrl("https://sub.id.scu.edu.cn/profile")), QString("SESSION=abc"));
    QCOMPARE(jar.cookieHeader(QUrl("https://zhjw.scu.edu.cn/")), QString());
}

// 验证 CookieJar 的域名匹配和解析行为。
void PersonBFoundationTest::cookieJarParsesCommaSeparatedSetCookie()
{
    CookieJar jar;
    jar.storeFromSetCookie(
        QUrl("https://id.scu.edu.cn/login"),
        {QByteArray("SESSION=abc; Path=/; HttpOnly, lang=zh_CN; Path=/; SameSite=Lax")});

    const QString header = jar.cookieHeader(QUrl("https://id.scu.edu.cn/"));
    QVERIFY(header.contains("SESSION=abc"));
    QVERIFY(header.contains("lang=zh_CN"));
}

// 验证 Cookie 调试摘要仅包含排序后的唯一名称和实际存储对数。
void PersonBFoundationTest::cookieJarDebugSummaryCountsPairsAndExcludesHostsAndValues()
{
    CookieJar jar;
    jar.storeFromSetCookie(
        QUrl(QStringLiteral("https://id.scu.edu.cn/login")),
        {QByteArrayLiteral("TGC=first-secret; Path=/, JSESSIONID=second-secret; Path=/")});

    QCOMPARE(jar.cookieSummaryForDebug(), QStringLiteral("count=2; names=JSESSIONID,TGC"));

    jar.storeFromSetCookie(QUrl(QStringLiteral("https://zhjw.scu.edu.cn/login")),
                           {QByteArrayLiteral("TGC=third-secret; Path=/")});
    const QString summary = jar.cookieSummaryForDebug();

    QCOMPARE(summary, QStringLiteral("count=3; names=JSESSIONID,TGC"));
    QVERIFY(!summary.contains(QStringLiteral("first-secret")));
    QVERIFY(!summary.contains(QStringLiteral("second-secret")));
    QVERIFY(!summary.contains(QStringLiteral("third-secret")));
    QVERIFY(!summary.contains(QStringLiteral("id.scu.edu.cn")));
    QVERIFY(!summary.contains(QStringLiteral("zhjw.scu.edu.cn")));
}

// 验证 CookieHttpClient 的网络响应处理行为。
void PersonBFoundationTest::cookieHttpClientReturnsHttpErrorResponsesWithBody()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    const QByteArray body = R"({"success":false,"message":"invalid_captcha"})";
    int requestCount = 0;
    connect(&server, &QTcpServer::newConnection, &server, [&]() {
        while (server.hasPendingConnections()) {
            QTcpSocket* socket = server.nextPendingConnection();
            socket->setParent(&server);
            connect(socket, &QTcpSocket::readyRead, socket, [socket, body, &requestCount]() {
                if (!socket->readAll().contains("\r\n\r\n")) {
                    return;
                }
                ++requestCount;
                const QByteArray response = QByteArrayLiteral("HTTP/1.1 400 Bad Request\r\n")
                    + QByteArrayLiteral("Content-Type: application/json\r\n")
                    + QByteArrayLiteral("Content-Length: ") + QByteArray::number(body.size())
                    + QByteArrayLiteral("\r\nConnection: close\r\n\r\n")
                    + body;
                socket->write(response);
                socket->disconnectFromHost();
            });
        }
    });

    CookieHttpClient client;
    HttpResponse response;
    ApiError error;
    QEventLoop loop;

    client.post(QUrl(QStringLiteral("http://127.0.0.1:%1/rest_token").arg(server.serverPort())),
                QByteArrayLiteral("{}"),
                [&](const HttpResponse& receivedResponse, const ApiError& receivedError) {
        response = receivedResponse;
        error = receivedError;
        loop.quit();
    }, {
        {QStringLiteral("Content-Type"), QStringLiteral("application/json;charset=UTF-8")},
    });

    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    loop.exec();

    QCOMPARE(requestCount, 1);
    QCOMPARE(error.type, ApiErrorType::Unknown);
    QCOMPARE(response.statusCode, 400);
    QCOMPARE(response.body, body);
}

// 验证 CookieHttpClient 的网络响应处理行为。
void PersonBFoundationTest::cookieHttpClientReportsRedirectLimit()
{
    AuthLogger::instance().clear();
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    int requestCount = 0;
    connect(&server, &QTcpServer::newConnection, &server, [&]() {
        while (server.hasPendingConnections()) {
            QTcpSocket* socket = server.nextPendingConnection();
            socket->setParent(&server);
            connect(socket, &QTcpSocket::readyRead, socket, [socket, &requestCount]() {
                if (!socket->readAll().contains("\r\n\r\n")) {
                    return;
                }
                ++requestCount;
                const QByteArray response = QByteArrayLiteral("HTTP/1.1 302 Found\r\n")
                    + QByteArrayLiteral("Location: /loop\r\n")
                    + QByteArrayLiteral("Set-Cookie: hop=") + QByteArray::number(requestCount)
                    + QByteArrayLiteral("; Path=/\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
                socket->write(response);
                socket->disconnectFromHost();
            });
        }
    });

    CookieHttpClient client;
    HttpResponse response;
    ApiError error;
    QEventLoop loop;

    client.followRedirects(QUrl(QStringLiteral("http://127.0.0.1:%1/start").arg(server.serverPort())),
                           [&](const HttpResponse& receivedResponse, const ApiError& receivedError) {
        response = receivedResponse;
        error = receivedError;
        loop.quit();
    }, {}, 1);

    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    loop.exec();

    QCOMPARE(requestCount, 2);
    QCOMPARE(error.type, ApiErrorType::ServiceUnavailable);
    QVERIFY(error.message.contains(QStringLiteral("重定向")));
    QCOMPARE(response.statusCode, 0);

    const auto logs = AuthLogger::instance().entries();
    QVERIFY(std::any_of(logs.cbegin(), logs.cend(), [](const AuthLogEntry& entry) {
        return entry.tag == QStringLiteral("CookieHttpClient")
            && entry.message.contains(QStringLiteral("redirect hop=0"));
    }));
}

// 验证 CookieHttpClient 的网络响应处理行为。
void PersonBFoundationTest::cookieHttpClientSendsCookiesStoredDuringRedirect()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    bool finalRequestHadCookie = false;
    connect(&server, &QTcpServer::newConnection, &server, [&]() {
        while (server.hasPendingConnections()) {
            QTcpSocket* socket = server.nextPendingConnection();
            socket->setParent(&server);
            connect(socket, &QTcpSocket::readyRead, socket, [socket, &finalRequestHadCookie]() {
                const QByteArray request = socket->readAll();
                if (!request.contains("\r\n\r\n")) {
                    return;
                }
                if (request.startsWith("GET /start ")) {
                    const QByteArray response = QByteArrayLiteral("HTTP/1.1 302 Found\r\n")
                        + QByteArrayLiteral("Location: /done\r\n")
                        + QByteArrayLiteral("Set-Cookie: SESSION=abc; Path=/; HttpOnly\r\n")
                        + QByteArrayLiteral("Content-Length: 0\r\nConnection: close\r\n\r\n");
                    socket->write(response);
                    socket->disconnectFromHost();
                    return;
                }

                finalRequestHadCookie = request.contains("Cookie: SESSION=abc");
                const QByteArray body = "ok";
                const QByteArray response = QByteArrayLiteral("HTTP/1.1 200 OK\r\n")
                    + QByteArrayLiteral("Content-Type: text/plain\r\n")
                    + QByteArrayLiteral("Content-Length: ") + QByteArray::number(body.size())
                    + QByteArrayLiteral("\r\nConnection: close\r\n\r\n")
                    + body;
                socket->write(response);
                socket->disconnectFromHost();
            });
        }
    });

    CookieHttpClient client;
    HttpResponse response;
    ApiError error;
    QEventLoop loop;

    client.followRedirects(QUrl(QStringLiteral("http://127.0.0.1:%1/start").arg(server.serverPort())),
                           [&](const HttpResponse& receivedResponse, const ApiError& receivedError) {
        response = receivedResponse;
        error = receivedError;
        loop.quit();
    }, {}, 2);

    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    loop.exec();

    QCOMPARE(error.type, ApiErrorType::Unknown);
    QCOMPARE(response.statusCode, 200);
    QCOMPARE(response.body, QByteArray("ok"));
    QVERIFY(finalRequestHadCookie);
    QCOMPARE(client.cookieSummaryForDebug(), QStringLiteral("count=1; names=SESSION"));
}

// 验证认证日志记录与脱敏行为。
void PersonBFoundationTest::authLoggerRedactsSensitiveValues()
{
    AuthLogger logger(10);

    logger.info(QStringLiteral("ScuAuth"),
                QStringLiteral(R"(payload {"access_token":"abc.def","password":"secret"} Authorization: Bearer token-value&code=abcdefg)"));

    QCOMPARE(logger.entries().size(), 1);
    const QString message = logger.entries().first().message;
    QVERIFY(message.contains(QStringLiteral(R"("access_token":"<redacted>")")));
    QVERIFY(message.contains(QStringLiteral(R"("password":"<redacted>")")));
    QVERIFY(message.contains(QStringLiteral("Bearer <redacted>")));
    QVERIFY(message.contains(QStringLiteral("code=abcd...")));
    QVERIFY(!message.contains(QStringLiteral("abc.def")));
    QVERIFY(!message.contains(QStringLiteral("secret")));
    QVERIFY(!message.contains(QStringLiteral("token-value")));
}

// 验证身份、验证码和 Cookie 字段不会出现在认证日志中。
void PersonBFoundationTest::authLoggerRedactsIdentityCaptchaAndCookieValues()
{
    const QString redacted = AuthLogRedactor::apply(
        QStringLiteral("username=202312345678 studentId: 202398765432 captcha=ABCD Cookie: TGC=secret"));

    QCOMPARE(redacted,
             QStringLiteral("username=<redacted> studentId: <redacted> captcha=<redacted> Cookie: <redacted>"));
    QVERIFY(!redacted.contains(QStringLiteral("202312345678")));
    QVERIFY(!redacted.contains(QStringLiteral("202398765432")));
    QVERIFY(!redacted.contains(QStringLiteral("ABCD")));
    QVERIFY(!redacted.contains(QStringLiteral("secret")));
}

// 验证 JSON token 与表单 password 字段按大小写无关方式脱敏。
void PersonBFoundationTest::authLoggerRedactsJsonTokenAndFormPasswordCaseInsensitively()
{
    const QString redacted = AuthLogRedactor::apply(
        QStringLiteral("payload {\"ToKeN\":\"json-secret\"} PASSWORD=form-secret&continue=1"));

    QCOMPARE(redacted,
             QStringLiteral("payload {\"ToKeN\":\"<redacted>\"} PASSWORD=<redacted>&continue=1"));
    QVERIFY(!redacted.contains(QStringLiteral("json-secret")));
    QVERIFY(!redacted.contains(QStringLiteral("form-secret")));
}

// 验证认证日志记录与脱敏行为。
void PersonBFoundationTest::authLoggerCapsRingBuffer()
{
    AuthLogger logger(2);

    logger.debug(QStringLiteral("Test"), QStringLiteral("first"));
    logger.warn(QStringLiteral("Test"), QStringLiteral("second"));
    logger.error(QStringLiteral("Test"), QStringLiteral("third"));

    QCOMPARE(logger.entries().size(), 2);
    QCOMPARE(logger.entries().at(0).message, QStringLiteral("second"));
    QCOMPARE(logger.entries().at(1).message, QStringLiteral("third"));
    QCOMPARE(logger.entries().at(1).level, AuthLogLevel::Error);
}

// 验证综合教务页面解析器行为。
void PersonBFoundationTest::zhjwParsersExtractRequiredValues()
{
    QCOMPARE(ZhjwParsers::parseCurrentWeek("<span>当前是第16周</span>"), 16);

    const auto semesters = ZhjwParsers::parseSemesters(
        R"(<select><option value="2025-2026-2-1">2025-2026-2（当前）</option><option value="2024-2025-1-1">2024-2025-1</option></select>)");
    QCOMPARE(semesters.size(), 2);
    QCOMPARE(semesters.at(0).value, QString("2025-2026-2-1"));
    QCOMPARE(semesters.at(0).label, QString("2025-2026-2（当前）"));

    const QString examHtml = QString::fromUtf8(R"(
        <div class="widget-box widget-color-blue">
          <h5 class="widget-title smaller">高等数学（已结束）</h5>
          <div>第18周</div><div>2026-01-09</div><div>星期五</div><div>09:00-11:00</div>
          <div>地点:&nbsp;江安一教 A101</div>
          <div>座位号:&nbsp;32</div>
          <div>准考证号:&nbsp;ABC123</div>
          <div>考试提示信息：&nbsp;携带证件</div>
        </div>)");
    const auto exams = ZhjwParsers::parseExamPlan(examHtml);
    QCOMPARE(exams.size(), 1);
    QCOMPARE(exams.at(0).courseName, QString("高等数学"));
    QCOMPARE(exams.at(0).week, QString("18"));
    QCOMPARE(exams.at(0).date, QString("2026-01-09"));
    QCOMPARE(exams.at(0).weekday, QString("星期五"));
    QCOMPARE(exams.at(0).timeRange, QString("09:00-11:00"));
    QCOMPARE(exams.at(0).location, QString("江安一教 A101"));
    QCOMPARE(exams.at(0).seatNumber, QString("32"));
    QCOMPARE(exams.at(0).ticketNumber, QString("ABC123"));
    QCOMPARE(exams.at(0).tip, QString("携带证件"));

    QCOMPARE(ZhjwParsers::extractSchemeScoresCallback(
                 R"(var url = "/student/integratedQuery/scoreQuery/abc/schemeScores/callback")"),
             QString("/student/integratedQuery/scoreQuery/abc/schemeScores/callback"));
    QCOMPARE(ZhjwParsers::extractPassingScoresCallback(
                 R"(var url = "/student/integratedQuery/scoreQuery/abc/allPassingScores/callback")"),
             QString("/student/integratedQuery/scoreQuery/abc/allPassingScores/callback"));
}

// 验证有效考表组件会被识别并解析为条目。
void PersonBFoundationTest::zhjwExamPlanParserRecognizesValidWidget()
{
    const QString html = QString::fromUtf8(R"(
        <div class="widget-box widget-color-blue">
          <h5 class="widget-title smaller">线性代数</h5>
          <div>第18周</div><div>2026-01-09</div><div>星期五</div><div>09:00-11:00</div>
        </div>)");

    const ZhjwParsers::ExamPlanParseResult result = ZhjwParsers::parseExamPlanResult(html);

    QVERIFY(result.recognized);
    QVERIFY(!result.explicitlyEmpty);
    QCOMPARE(result.items.size(), 1);
    QCOMPARE(result.items.first().courseName, QStringLiteral("线性代数"));
}

// 验证教务系统的三种明确空结果标记都会返回成功空态。
void PersonBFoundationTest::zhjwExamPlanParserRecognizesExplicitEmptyMarkers()
{
    const QStringList markers = {
        QStringLiteral("暂无考试安排"),
        QStringLiteral("暂无数据"),
        QStringLiteral("没有查询到"),
    };

    for (const QString& marker : markers) {
        const ZhjwParsers::ExamPlanParseResult result =
            ZhjwParsers::parseExamPlanResult(QStringLiteral("<html><body>%1</body></html>").arg(marker));
        QVERIFY2(result.items.isEmpty(), qPrintable(marker));
        QVERIFY2(!result.recognized, qPrintable(marker));
        QVERIFY2(result.explicitlyEmpty, qPrintable(marker));
    }
}

// 验证无考表结构或空结果标记的页面不会被误判为成功。
void PersonBFoundationTest::zhjwExamPlanParserRejectsUnrelatedHtml()
{
    const ZhjwParsers::ExamPlanParseResult result =
        ZhjwParsers::parseExamPlanResult(QStringLiteral("<html><body>教务系统维护中</body></html>"));

    QVERIFY(result.items.isEmpty());
    QVERIFY(!result.recognized);
    QVERIFY(!result.explicitlyEmpty);
}

// 验证组件结构仍可识别但字段改版无法解析时，不会伪装成明确空结果。
void PersonBFoundationTest::zhjwExamPlanParserRecognizesUnparseableWidget()
{
    const ZhjwParsers::ExamPlanParseResult result = ZhjwParsers::parseExamPlanResult(
        QStringLiteral("<div class=\"widget-box widget-color-red\"><h5 class=\"renamed-title\">大学物理</h5></div>"));

    QVERIFY(result.items.isEmpty());
    QVERIFY(result.recognized);
    QVERIFY(!result.explicitlyEmpty);
}

void PersonBFoundationTest::zhjwParsersReadClassroomIndexAndAvailability()
{
    QFile indexFile(QFINDTESTDATA("fixtures/classroom_index.html"));
    QVERIFY(indexFile.open(QIODevice::ReadOnly));
    ClassroomIndexDto index;
    QString error;
    QVERIFY2(ZhjwParsers::parseClassroomIndex(QString::fromUtf8(indexFile.readAll()),
                                              &index,
                                              &error),
             qPrintable(error));
    QCOMPARE(index.campuses.size(), 2);
    QCOMPARE(index.campuses.first().campusName, QStringLiteral("江安"));
    QCOMPARE(index.buildings.first().teachingBuildingNumber, QStringLiteral("101"));

    QFile queryFile(QFINDTESTDATA("fixtures/classroom_query.json"));
    QVERIFY(queryFile.open(QIODevice::ReadOnly));
    ClassroomQueryResultDto result;
    QVERIFY2(ZhjwParsers::parseClassroomQuery(queryFile.readAll(), &result, &error),
             qPrintable(error));
    QCOMPARE(result.classrooms.size(), 1);
    QCOMPARE(result.classrooms.first().placeNum, 120);
    QCOMPARE(result.timeSlots.first().continuingSession, 2);
    QCOMPARE(result.teachingWeek, 20);
}

void PersonBFoundationTest::zhjwParsersRejectMalformedClassroomPayloads()
{
    ClassroomIndexDto index;
    ClassroomQueryResultDto result;
    QString error;

    QVERIFY(!ZhjwParsers::parseClassroomIndex(QStringLiteral("<html>维护中</html>"),
                                              &index,
                                              &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(!ZhjwParsers::parseClassroomQuery(QByteArrayLiteral("[]"), &result, &error));
    QVERIFY(!error.isEmpty());
}

// 验证综合教务页面解析器行为。
void PersonBFoundationTest::zhjwParsersDetectExpiredSessions()
{
    QVERIFY(ZhjwParsers::isSessionExpired("", 200));
    QVERIFY(ZhjwParsers::isSessionExpired("<html>login</html>", 200));
    QVERIFY(ZhjwParsers::isSessionExpired("<html>统一身份认证 用户登录</html>", 200));
    QVERIFY(ZhjwParsers::isSessionExpired("redirect", 302));
    QVERIFY(!ZhjwParsers::isSessionExpired("{\"success\":true}", 200));
}

// 验证认证视图模型的状态和信号行为。
void PersonBFoundationTest::authViewModelWritesFreshCaptchaUrlEachTime()
{
    AuthViewModel model;

    const QUrl first = model.writeCaptchaImage(QByteArray("first-image"), QStringLiteral("image/png"));
    const QUrl second = model.writeCaptchaImage(QByteArray("second-image"), QStringLiteral("image/png"));

    QVERIFY(first.isLocalFile());
    QVERIFY(second.isLocalFile());
    QVERIFY(first != second);
}

// 验证认证视图模型的状态和信号行为。
void PersonBFoundationTest::authViewModelRejectsLoginBeforeCaptchaLoaded()
{
    AuthViewModel model;

    model.login(QStringLiteral("20260001"), QStringLiteral("secret"), QStringLiteral("abcd"));

    QCOMPARE(model.loading(), false);
    QCOMPARE(model.errorMessage(), QStringLiteral("请先刷新验证码"));
}

// 验证认证视图模型的状态和信号行为。
void PersonBFoundationTest::authViewModelDoesNotEmitLoginFailedForCaptchaFetchErrors()
{
    FakeScuAuthService service;
    AuthViewModel model(&service);
    QSignalSpy loginFailedSpy(&model, &AuthViewModel::loginFailed);

    model.fetchCaptcha();

    QCOMPARE(loginFailedSpy.count(), 0);
    QCOMPARE(model.errorMessage(), QStringLiteral("captcha down"));
    QCOMPARE(model.captchaLoading(), false);
}

// 验证认证视图模型的状态和信号行为。
void PersonBFoundationTest::authViewModelForwardsSessionExpired()
{
    FakeScuAuthService service;
    service.saveTokenForTesting(QStringLiteral("access-token-1"), QDateTime::currentSecsSinceEpoch());
    AuthViewModel model(&service);
    QVERIFY(model.loggedIn());
    QSignalSpy expiredSpy(&model, &AuthViewModel::sessionExpired);

    service.triggerSessionExpired(QStringLiteral("登录已过期，请重新登录"));

    QCOMPARE(model.loggedIn(), false);
    QCOMPARE(model.errorMessage(), QStringLiteral("登录已过期，请重新登录"));
    QCOMPARE(expiredSpy.count(), 1);
}

// 验证统一身份认证服务的核心流程。
void PersonBFoundationTest::scuAuthParsesCaptchaDataUrl()
{
    const QByteArray body = R"({
        "success": true,
        "data": {
            "code": "captcha-code-1",
            "captcha": "data:image/png;base64,QUJDRA=="
        }
    })";

    const CaptchaResult result = ScuAuthService::parseCaptchaResponse(body);
    QCOMPARE(result.code, QString("captcha-code-1"));
    QCOMPARE(result.mimeType, QString("image/png"));
    QCOMPARE(result.imageBytes, QByteArray("ABCD"));
}

// 验证统一身份认证服务的核心流程。
void PersonBFoundationTest::scuAuthTokenTtlAndStorageWork()
{
    QVERIFY(ScuAuthService::isTokenExpired(0, 5000));
    QVERIFY(!ScuAuthService::isTokenExpired(1000, 4599));
    QVERIFY(ScuAuthService::isTokenExpired(1000, 4601));

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "SCU_Nexus_Test", "AuthStore");
    settings.clear();

    ScuAuthService service(nullptr, &settings);
    service.saveTokenForTesting("token-value", 1234);
    QCOMPARE(service.accessToken(), QString("token-value"));
    QCOMPARE(service.loginTimestamp(), qint64(1234));

    service.logout();
    QCOMPARE(service.accessToken(), QString());
    QCOMPARE(service.loginTimestamp(), qint64(0));
}

// 验证综合教务 API 服务的请求与重试行为。
void PersonBFoundationTest::zhjwApiExposesDocumentedEndpoints()
{
    QCOMPARE(ZhjwApiService::scuIdBase(), QString("https://id.scu.edu.cn"));
    QCOMPARE(ZhjwApiService::zhjwBase(), QString("http://zhjw.scu.edu.cn"));
    QCOMPARE(ZhjwApiService::jwcCalendarUrl(), QString("https://jwc.scu.edu.cn/cdxl.htm"));
    QVERIFY(ZhjwApiService::isSessionExpired("<html>统一身份认证 用户登录</html>", 200));
}

void PersonBFoundationTest::zhjwApiRequestsClassroomIndexAndAvailability()
{
    auto* client = new FakeCookieHttpClient();
    const QString indexUrl = QStringLiteral(
        "http://zhjw.scu.edu.cn/student/teachingResources/classroomUseStatus/index");
    const QString queryUrl = QStringLiteral(
        "http://zhjw.scu.edu.cn/student/teachingResources/classroomUseStatus/jasInfo");

    QFile indexFile(QFINDTESTDATA("fixtures/classroom_index.html"));
    QFile queryFile(QFINDTESTDATA("fixtures/classroom_query.json"));
    QVERIFY(indexFile.open(QIODevice::ReadOnly));
    QVERIFY(queryFile.open(QIODevice::ReadOnly));

    HttpResponse indexResponse;
    indexResponse.statusCode = 200;
    indexResponse.body = indexFile.readAll();
    indexResponse.finalUrl = QUrl(indexUrl);
    client->responses[indexUrl] = indexResponse;

    HttpResponse queryResponse;
    queryResponse.statusCode = 200;
    queryResponse.body = queryFile.readAll();
    queryResponse.finalUrl = QUrl(queryUrl);
    client->responses[queryUrl] = queryResponse;

    FakeZhjwAuthService auth(client);
    ZhjwApiService api(nullptr, &auth);
    ClassroomIndexDto index;
    ClassroomQueryResultDto result;
    ApiError indexError;
    ApiError queryError;
    api.fetchClassroomIndex(
        [&](const ClassroomIndexDto& value, const ApiError& error) {
            index = value;
            indexError = error;
        });
    api.fetchClassroomAvailability(
        QStringLiteral("01"),
        QStringLiteral("101"),
        QStringLiteral("2026-07-15"),
        [&](const ClassroomQueryResultDto& value, const ApiError& error) {
            result = value;
            queryError = error;
        });

    QCOMPARE(indexError.type, ApiErrorType::Unknown);
    QCOMPARE(queryError.type, ApiErrorType::Unknown);
    QCOMPARE(client->getUrls, QStringList{indexUrl});
    QCOMPARE(client->postUrls, QStringList{queryUrl});
    QVERIFY(client->postBodies.first().contains("xqh=01"));
    QVERIFY(client->postBodies.first().contains("jxlh=101"));
    QVERIFY(client->postBodies.first().contains("searchDate=2026-07-15"));
    QCOMPARE(client->postHeaders.first().value(QStringLiteral("X-Requested-With")),
             QStringLiteral("XMLHttpRequest"));
    QCOMPARE(index.campuses.size(), 2);
    QCOMPARE(result.classrooms.size(), 1);
}

// 验证 SM2 加密输出格式符合接口要求。
void PersonBFoundationTest::sm2CryptoEncryptsToC1C2C3Base64()
{
    const QString publicKeyBase64 =
        "BMKVod3cxpgT8CSv8v67UEdOTC2pMKIkX/+Vvn6T7EZPE2EbT5xBDbw95ribJAQbngNjGQv9KieqB12MUG3koDE=";
    const QString encrypted = Sm2Crypto::encryptWithBase64Key("secret", publicKeyBase64);
    const QByteArray bytes = QByteArray::fromBase64(encrypted.toUtf8());

    QVERIFY(!encrypted.isEmpty());
    QVERIFY(bytes.size() >= 1 + 64 + 6 + 32);
    QCOMPARE(static_cast<unsigned char>(bytes.at(0)), static_cast<unsigned char>(0x04));
}

// 验证统一身份认证服务的核心流程。
void PersonBFoundationTest::scuAuthLoginRequestsSm2ThenTokenAndStoresToken()
{
    AuthLogger::instance().clear();
    auto* fakeClient = new FakeCookieHttpClient();
    HttpResponse sm2Response;
    sm2Response.statusCode = 200;
    sm2Response.finalUrl = QUrl("https://id.scu.edu.cn/api/public/bff/v1.2/sm2_key");
    sm2Response.body = R"({
        "success": true,
        "data": {
            "publicKey": "BMKVod3cxpgT8CSv8v67UEdOTC2pMKIkX/+Vvn6T7EZPE2EbT5xBDbw95ribJAQbngNjGQv9KieqB12MUG3koDE=",
            "code": "sm2-code-1"
        }
    })";

    HttpResponse tokenResponse;
    tokenResponse.statusCode = 200;
    tokenResponse.finalUrl = QUrl("https://id.scu.edu.cn/api/public/bff/v1.2/rest_token");
    tokenResponse.body = R"({
        "success": true,
        "data": {
            "access_token": "access-token-1"
        }
    })";

    fakeClient->responses.insert(sm2Response.finalUrl.toString(), sm2Response);
    fakeClient->responses.insert(tokenResponse.finalUrl.toString(), tokenResponse);

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "SCU_Nexus_Test", "LoginFlow");
    settings.clear();
    ScuAuthService service(nullptr, &settings, [fakeClient](QObject* parent) {
        fakeClient->setParent(parent);
        return fakeClient;
    });

    ApiError result;
    service.login("20260001", "secret", "captcha-code-1", "abcd", [&result](const ApiError& error) {
        result = error;
    });

    QCOMPARE(result.type, ApiErrorType::Unknown);
    QCOMPARE(service.accessToken(), QString("access-token-1"));
    QCOMPARE(fakeClient->postUrls.size(), 2);
    QVERIFY(fakeClient->postUrls.at(0).endsWith("/sm2_key"));
    QVERIFY(fakeClient->postUrls.at(1).endsWith("/rest_token"));

    const QJsonObject payload = QJsonDocument::fromJson(fakeClient->postBodies.at(1)).object();
    QCOMPARE(payload.value("username").toString(), QString("20260001"));
    QCOMPARE(payload.value("sm2_code").toString(), QString("sm2-code-1"));
    QCOMPARE(payload.value("cap_code").toString(), QString("captcha-code-1"));
    QCOMPARE(payload.value("cap_text").toString(), QString("abcd"));
    QVERIFY(!payload.value("password").toString().isEmpty());
    QVERIFY(payload.value("password").toString() != QString("secret"));

    const auto logs = AuthLogger::instance().entries();
    QVERIFY(std::any_of(logs.cbegin(), logs.cend(), [](const AuthLogEntry& entry) {
        return entry.tag == QStringLiteral("ScuAuth")
            && entry.message.contains(QStringLiteral("token acquired len=14"));
    }));
    QVERIFY(std::none_of(logs.cbegin(), logs.cend(), [](const AuthLogEntry& entry) {
        return entry.message.contains(QStringLiteral("access-token-1"));
    }));
    QVERIFY(std::none_of(logs.cbegin(), logs.cend(), [](const AuthLogEntry& entry) {
        return entry.message.contains(QStringLiteral("20260001"));
    }));
}

// 验证统一身份认证服务的核心流程。
void PersonBFoundationTest::scuAuthLoginParsesRestTokenHttp400JsonError()
{
    auto* fakeClient = new FakeCookieHttpClient();
    HttpResponse sm2Response;
    sm2Response.statusCode = 200;
    sm2Response.finalUrl = QUrl("https://id.scu.edu.cn/api/public/bff/v1.2/sm2_key");
    sm2Response.body = R"({
        "success": true,
        "data": {
            "publicKey": "BMKVod3cxpgT8CSv8v67UEdOTC2pMKIkX/+Vvn6T7EZPE2EbT5xBDbw95ribJAQbngNjGQv9KieqB12MUG3koDE=",
            "code": "sm2-code-1"
        }
    })";

    HttpResponse tokenResponse;
    tokenResponse.statusCode = 400;
    tokenResponse.finalUrl = QUrl("https://id.scu.edu.cn/api/public/bff/v1.2/rest_token");
    tokenResponse.body = R"({
        "success": false,
        "message": "invalid_captcha"
    })";

    fakeClient->responses.insert(sm2Response.finalUrl.toString(), sm2Response);
    fakeClient->responses.insert(tokenResponse.finalUrl.toString(), tokenResponse);

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "SCU_Nexus_Test", "LoginHttp400");
    settings.clear();
    ScuAuthService service(nullptr, &settings, [fakeClient](QObject* parent) {
        fakeClient->setParent(parent);
        return fakeClient;
    });

    ApiError result;
    service.login("20260001", "secret", "captcha-code-1", "bad", [&result](const ApiError& error) {
        result = error;
    });

    QCOMPARE(result.type, ApiErrorType::InvalidCaptcha);
    QCOMPARE(result.message, QString("invalid_captcha"));
    QCOMPARE(result.statusCode, 400);
    QVERIFY(result.debugBody.contains("invalid_captcha"));
    QCOMPARE(service.accessToken(), QString());
}

// 验证统一身份认证服务的核心流程。
void PersonBFoundationTest::scuAuthBindSessionCoalescesConcurrentCalls()
{
    QList<DeferredSessionClient*> clients;
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "SCU_Nexus_Test", "BindSessionCoalescing");
    settings.clear();
    ScuAuthService service(nullptr, &settings, [&clients](QObject* parent) {
        auto* client = new DeferredSessionClient(parent);
        clients.append(client);
        return client;
    });
    service.saveTokenForTesting(QStringLiteral("access-token-1"), QDateTime::currentSecsSinceEpoch());

    int callbacks = 0;
    CookieHttpClient* firstClient = nullptr;
    CookieHttpClient* secondClient = nullptr;
    ApiError firstError;
    ApiError secondError;

    service.bindSession([&](CookieHttpClient* client, const ApiError& error) {
        ++callbacks;
        firstClient = client;
        firstError = error;
    });
    service.bindSession([&](CookieHttpClient* client, const ApiError& error) {
        ++callbacks;
        secondClient = client;
        secondError = error;
    });

    QCOMPARE(clients.size(), 1);
    QCOMPARE(clients.first()->postUrls.size(), 1);
    QCOMPARE(callbacks, 0);

    clients.first()->completeNextSessionSave();

    QCOMPARE(callbacks, 2);
    QCOMPARE(firstError.type, ApiErrorType::Unknown);
    QCOMPARE(secondError.type, ApiErrorType::Unknown);
    QCOMPARE(firstClient, clients.first());
    QCOMPARE(secondClient, clients.first());
}

// 验证统一身份认证服务的核心流程。
void PersonBFoundationTest::scuAuthBindSessionRefreshesExpiredTokenWhenSessionSaveSucceeds()
{
    QList<DeferredSessionClient*> clients;
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "SCU_Nexus_Test", "BindSessionRefreshExpired");
    settings.clear();
    ScuAuthService service(nullptr, &settings, [&clients](QObject* parent) {
        auto* client = new DeferredSessionClient(parent);
        clients.append(client);
        return client;
    });
    const qint64 expiredTimestamp = QDateTime::currentSecsSinceEpoch() - 3700;
    service.saveTokenForTesting(QStringLiteral("access-token-1"), expiredTimestamp);
    QSignalSpy expiredSpy(&service, &ScuAuthService::sessionExpired);

    int callbacks = 0;
    CookieHttpClient* refreshedClient = nullptr;
    ApiError result;

    service.bindSession([&](CookieHttpClient* client, const ApiError& error) {
        ++callbacks;
        refreshedClient = client;
        result = error;
    });

    QCOMPARE(clients.size(), 1);
    QCOMPARE(clients.first()->postUrls.size(), 1);
    QCOMPARE(callbacks, 0);

    clients.first()->completeNextSessionSave();

    QCOMPARE(callbacks, 1);
    QCOMPARE(result.type, ApiErrorType::Unknown);
    QCOMPARE(refreshedClient, clients.first());
    QVERIFY(service.loginTimestamp() > expiredTimestamp);
    QVERIFY(service.loggedIn());
    QCOMPARE(expiredSpy.count(), 0);
}

// 验证综合教务认证会话管理行为。
void PersonBFoundationTest::zhjwAuthCoalescesConcurrentSsoLogin()
{
    AuthLogger::instance().clear();
    DeferredRedirectClient client;
    BoundScuAuthService scuAuth(&client);
    ZhjwAuthService auth(nullptr, &scuAuth);

    int callbacks = 0;
    CookieHttpClient* firstClient = nullptr;
    CookieHttpClient* secondClient = nullptr;
    ApiError firstError;
    ApiError secondError;

    auth.getClient([&](CookieHttpClient* receivedClient, const ApiError& error) {
        ++callbacks;
        firstClient = receivedClient;
        firstError = error;
    });
    auth.getClient([&](CookieHttpClient* receivedClient, const ApiError& error) {
        ++callbacks;
        secondClient = receivedClient;
        secondError = error;
    });

    QCOMPARE(scuAuth.bindSessionCalls, 1);
    QCOMPARE(client.redirectUrls.size(), 1);
    QCOMPARE(callbacks, 0);

    client.completeNextRedirect();

    QCOMPARE(callbacks, 2);
    QCOMPARE(firstError.type, ApiErrorType::Unknown);
    QCOMPARE(secondError.type, ApiErrorType::Unknown);
    QCOMPARE(firstClient, &client);
    QCOMPARE(secondClient, &client);

    const auto logs = AuthLogger::instance().entries();
    QVERIFY(std::any_of(logs.cbegin(), logs.cend(), [](const AuthLogEntry& entry) {
        return entry.tag == QStringLiteral("ZhjwAuth")
            && entry.message.contains(QStringLiteral("SSO login ok"));
    }));
}

// 验证综合教务认证会话管理行为。
void PersonBFoundationTest::zhjwAuthRebindsWhenScuTokenChanges()
{
    DeferredRedirectClient firstClient;
    DeferredRedirectClient secondClient;
    BoundScuAuthService scuAuth(&firstClient);
    ZhjwAuthService auth(nullptr, &scuAuth);

    int firstCallbacks = 0;
    CookieHttpClient* firstResult = nullptr;
    ApiError firstError;

    auth.getClient([&](CookieHttpClient* receivedClient, const ApiError& error) {
        ++firstCallbacks;
        firstResult = receivedClient;
        firstError = error;
    });

    QCOMPARE(scuAuth.bindSessionCalls, 1);
    QCOMPARE(firstClient.redirectUrls.size(), 1);
    firstClient.completeNextRedirect();

    QCOMPARE(firstCallbacks, 1);
    QCOMPARE(firstError.type, ApiErrorType::Unknown);
    QCOMPARE(firstResult, &firstClient);

    scuAuth.boundClient = &secondClient;
    scuAuth.saveTokenForTesting(QStringLiteral("access-token-2"), QDateTime::currentSecsSinceEpoch());

    int secondCallbacks = 0;
    CookieHttpClient* secondResult = nullptr;
    ApiError secondError;

    auth.getClient([&](CookieHttpClient* receivedClient, const ApiError& error) {
        ++secondCallbacks;
        secondResult = receivedClient;
        secondError = error;
    });

    QCOMPARE(scuAuth.bindSessionCalls, 2);
    QCOMPARE(secondClient.redirectUrls.size(), 1);
    QCOMPARE(secondCallbacks, 0);

    secondClient.completeNextRedirect();

    QCOMPARE(secondCallbacks, 1);
    QCOMPARE(secondError.type, ApiErrorType::Unknown);
    QCOMPARE(secondResult, &secondClient);
}

// 验证综合教务认证每次缓存命中前仍会先让统一认证层检查/刷新 session。
void PersonBFoundationTest::zhjwAuthRevalidatesScuSessionBeforeCacheHit()
{
    DeferredRedirectClient client;
    BoundScuAuthService scuAuth(&client);
    ZhjwAuthService auth(nullptr, &scuAuth);

    int firstCallbacks = 0;
    auth.getClient([&](CookieHttpClient* receivedClient, const ApiError& error) {
        ++firstCallbacks;
        QCOMPARE(error.type, ApiErrorType::Unknown);
        QCOMPARE(receivedClient, &client);
    });
    QCOMPARE(scuAuth.bindSessionCalls, 1);
    QCOMPARE(client.redirectUrls.size(), 1);
    client.completeNextRedirect();
    QCOMPARE(firstCallbacks, 1);

    scuAuth.saveTokenForTesting(QStringLiteral("access-token-1"), QDateTime::currentSecsSinceEpoch() - 3700);

    int secondCallbacks = 0;
    auth.getClient([&](CookieHttpClient* receivedClient, const ApiError& error) {
        ++secondCallbacks;
        QCOMPARE(error.type, ApiErrorType::Unknown);
        QCOMPARE(receivedClient, &client);
    });

    QCOMPARE(secondCallbacks, 1);
    QCOMPARE(scuAuth.bindSessionCalls, 2);
    QCOMPARE(client.redirectUrls.size(), 1);
}

// 验证主应用认证链可按 Bugaoshan 的单根 ScuAuth 模式共享同一实例。
void PersonBFoundationTest::appControllerCanShareScuAuthWithZhjwStack()
{
    FakeScuAuthService sharedAuth;
    AppController controller(nullptr, &sharedAuth);
    auto* authModel = qobject_cast<AuthViewModel*>(controller.authViewModel());
    QVERIFY(authModel);
    QCOMPARE(authModel->m_authService, &sharedAuth);

    ZhjwAuthService zhjwAuth(nullptr, &sharedAuth);
    ZhjwApiService api(nullptr, &zhjwAuth);
    ZhjwApiQueryService query(nullptr, &api);
    QCOMPARE(zhjwAuth.m_scuAuthService, &sharedAuth);
    QCOMPARE(api.m_authService, &zhjwAuth);
    QCOMPARE(query.m_api, &api);
}

// 验证启动时恢复的统一认证 token 会同步到 AppController 和查询服务入口。
void PersonBFoundationTest::appControllerReflectsRestoredAuthState()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "SCU_Nexus_Test", "AppControllerRestore");
    settings.clear();
    ScuAuthService sharedAuth(nullptr, &settings);
    sharedAuth.saveTokenForTesting(QStringLiteral("access-token-1"), QDateTime::currentSecsSinceEpoch());

    AppController controller(nullptr, &sharedAuth);
    auto* authModel = qobject_cast<AuthViewModel*>(controller.authViewModel());
    QVERIFY(authModel);
    QVERIFY(authModel->loggedIn());
    QVERIFY(controller.loggedIn());
}

// 验证考表页面请求头与 Bugaoshan 保持一致。
void PersonBFoundationTest::zhjwApiUsesBrowserAcceptForExamPlan()
{
    auto* fakeClient = new FakeCookieHttpClient();
    const QString examUrl = "http://zhjw.scu.edu.cn/student/examinationManagement/examPlan/index";

    HttpResponse examResponse;
    examResponse.statusCode = 200;
    examResponse.finalUrl = QUrl(examUrl);
    examResponse.body = QStringLiteral("<html><body>暂无考试安排</body></html>").toUtf8();
    fakeClient->responses.insert(examUrl, examResponse);

    FakeZhjwAuthService auth(fakeClient);
    ZhjwApiService service(nullptr, &auth);
    ApiError result;
    service.fetchExamPlan([&result](const QList<ExamPlanItemDto>&, const ApiError& error) {
        result = error;
    });

    QCOMPARE(result.type, ApiErrorType::Unknown);
    QCOMPARE(fakeClient->getUrls.size(), 1);
    QCOMPARE(fakeClient->getHeaders.first().value(QStringLiteral("Accept")),
             QStringLiteral("text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8"));
}

// 验证有效考表页面仍通过 API 服务返回解析后的条目。
void PersonBFoundationTest::zhjwApiReturnsValidExamPlan()
{
    auto* fakeClient = new FakeCookieHttpClient();
    const QString examUrl = QStringLiteral("http://zhjw.scu.edu.cn/student/examinationManagement/examPlan/index");

    HttpResponse response;
    response.statusCode = 200;
    response.finalUrl = QUrl(examUrl);
    response.body = QString::fromUtf8(R"(
        <div class="widget-box widget-color-blue">
          <h5 class="widget-title smaller">线性代数</h5>
          <div>第18周</div><div>2026-01-09</div>
        </div>)").toUtf8();
    fakeClient->responses.insert(examUrl, response);

    FakeZhjwAuthService auth(fakeClient);
    ZhjwApiService service(nullptr, &auth);
    QList<ExamPlanItemDto> items;
    ApiError result;
    service.fetchExamPlan([&items, &result](const QList<ExamPlanItemDto>& exams, const ApiError& error) {
        items = exams;
        result = error;
    });

    QCOMPARE(result.type, ApiErrorType::Unknown);
    QCOMPARE(items.size(), 1);
    QCOMPARE(items.first().courseName, QStringLiteral("线性代数"));
}

// 验证明确空结果页面通过 API 服务返回成功空列表。
void PersonBFoundationTest::zhjwApiReturnsExplicitlyEmptyExamPlan()
{
    auto* fakeClient = new FakeCookieHttpClient();
    const QString examUrl = QStringLiteral("http://zhjw.scu.edu.cn/student/examinationManagement/examPlan/index");

    HttpResponse response;
    response.statusCode = 200;
    response.finalUrl = QUrl(examUrl);
    response.body = QStringLiteral("<html><body>暂无数据</body></html>").toUtf8();
    fakeClient->responses.insert(examUrl, response);

    FakeZhjwAuthService auth(fakeClient);
    ZhjwApiService service(nullptr, &auth);
    QList<ExamPlanItemDto> items;
    ApiError result;
    service.fetchExamPlan([&items, &result](const QList<ExamPlanItemDto>& exams, const ApiError& error) {
        items = exams;
        result = error;
    });

    QCOMPARE(result.type, ApiErrorType::Unknown);
    QVERIFY(items.isEmpty());
}

// 验证无关页面会由 API 服务映射为解析失败，而不是成功空列表。
void PersonBFoundationTest::zhjwApiMapsUnrelatedExamPageToParseFailure()
{
    auto* fakeClient = new FakeCookieHttpClient();
    const QString examUrl = QStringLiteral("http://zhjw.scu.edu.cn/student/examinationManagement/examPlan/index");

    HttpResponse response;
    response.statusCode = 200;
    response.finalUrl = QUrl(examUrl);
    response.body = QStringLiteral("<html><body>教务系统维护中</body></html>").toUtf8();
    fakeClient->responses.insert(examUrl, response);

    FakeZhjwAuthService auth(fakeClient);
    ZhjwApiService service(nullptr, &auth);
    QList<ExamPlanItemDto> items;
    ApiError result;
    service.fetchExamPlan([&items, &result](const QList<ExamPlanItemDto>& exams, const ApiError& error) {
        items = exams;
        result = error;
    });

    QVERIFY(items.isEmpty());
    QCOMPARE(result.type, ApiErrorType::ParseFailed);
    QCOMPARE(result.message, QStringLiteral("考试安排页面结构无法识别"));
    QCOMPARE(result.statusCode, 200);
}

// 验证已识别组件在字段改版后解析不到条目时同样映射为解析失败。
void PersonBFoundationTest::zhjwApiMapsUnparseableExamWidgetToParseFailure()
{
    auto* fakeClient = new FakeCookieHttpClient();
    const QString examUrl = QStringLiteral("http://zhjw.scu.edu.cn/student/examinationManagement/examPlan/index");

    HttpResponse response;
    response.statusCode = 200;
    response.finalUrl = QUrl(examUrl);
    response.body = QStringLiteral(
        "<div class=\"widget-box widget-color-red\"><h5 class=\"renamed-title\">大学物理</h5></div>")
                        .toUtf8();
    fakeClient->responses.insert(examUrl, response);

    FakeZhjwAuthService auth(fakeClient);
    ZhjwApiService service(nullptr, &auth);
    QList<ExamPlanItemDto> items;
    ApiError result;
    service.fetchExamPlan([&items, &result](const QList<ExamPlanItemDto>& exams, const ApiError& error) {
        items = exams;
        result = error;
    });

    QVERIFY(items.isEmpty());
    QCOMPARE(result.type, ApiErrorType::ParseFailed);
    QCOMPARE(result.message, QStringLiteral("考试安排页面结构无法识别"));
}

// 验证考表解析失败摘要会先规整空白、再脱敏，并截断到 500 字符。
void PersonBFoundationTest::zhjwApiSanitizesExamParseFailureSummary()
{
    AuthLogger::instance().clear();
    auto* fakeClient = new FakeCookieHttpClient();
    const QString examUrl = QStringLiteral("http://zhjw.scu.edu.cn/student/examinationManagement/examPlan/index");
    const QString body = QStringLiteral("<html>\n  <body>\tusername=202612345678  ")
        + QString(700, QLatin1Char('x'))
        + QStringLiteral("\n </body> </html>");

    HttpResponse response;
    response.statusCode = 200;
    response.finalUrl = QUrl(examUrl);
    response.body = body.toUtf8();
    fakeClient->responses.insert(examUrl, response);

    FakeZhjwAuthService auth(fakeClient);
    ZhjwApiService service(nullptr, &auth);
    ApiError result;
    service.fetchExamPlan([&result](const QList<ExamPlanItemDto>&, const ApiError& error) {
        result = error;
    });

    const QString expectedSummary = AuthLogRedactor::apply(body.simplified()).left(500);
    QCOMPARE(result.type, ApiErrorType::ParseFailed);
    QCOMPARE(result.debugBody, expectedSummary);
    QCOMPARE(result.debugBody.size(), 500);
    QVERIFY(result.debugBody.contains(QStringLiteral("username=<redacted>")));
    QVERIFY(!result.debugBody.contains(QStringLiteral("202612345678")));
    QVERIFY(!result.debugBody.contains(QLatin1Char('\n')));
    QVERIFY(!result.debugBody.contains(QLatin1Char('\t')));
    QVERIFY(!result.debugBody.contains(QStringLiteral("  ")));

    const QList<AuthLogEntry> logs = AuthLogger::instance().entries();
    QVERIFY(std::none_of(logs.cbegin(), logs.cend(), [&body](const AuthLogEntry& entry) {
        return entry.message.contains(body) || entry.message.contains(QStringLiteral("202612345678"));
    }));
}

// 验证成绩 index 页请求头与 Bugaoshan 保持一致，避免服务端返回非预期页面导致 callback 提取失败。
void PersonBFoundationTest::zhjwApiUsesBrowserAcceptForScoreIndex()
{
    auto* fakeClient = new FakeCookieHttpClient();
    const QString indexUrl = "http://zhjw.scu.edu.cn/student/integratedQuery/scoreQuery/schemeScores/index";
    const QString callbackUrl = "http://zhjw.scu.edu.cn/student/integratedQuery/scoreQuery/abc/schemeScores/callback";

    HttpResponse indexResponse;
    indexResponse.statusCode = 200;
    indexResponse.finalUrl = QUrl(indexUrl);
    indexResponse.body = R"(var url = "/student/integratedQuery/scoreQuery/abc/schemeScores/callback")";
    fakeClient->responses.insert(indexUrl, indexResponse);

    HttpResponse callbackResponse;
    callbackResponse.statusCode = 200;
    callbackResponse.finalUrl = QUrl(callbackUrl);
    callbackResponse.body = R"({"lnList":[]})";
    fakeClient->responses.insert(callbackUrl, callbackResponse);

    FakeZhjwAuthService auth(fakeClient);
    ZhjwApiService service(nullptr, &auth);
    ApiError result;
    service.fetchSchemeScores([&result](const QJsonObject&, const ApiError& error) {
        result = error;
    });

    QCOMPARE(result.type, ApiErrorType::Unknown);
    QCOMPARE(fakeClient->getUrls.size(), 2);
    QCOMPARE(fakeClient->getHeaders.first().value(QStringLiteral("Accept")),
             QStringLiteral("text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8"));
    QCOMPARE(fakeClient->getHeaders.at(1).value(QStringLiteral("Accept")),
             QStringLiteral("application/json, text/plain, */*"));
}

// 验证成绩 index 页无法提取 callback 且带登录标记时，会按未认证重试一次。
void PersonBFoundationTest::zhjwApiRetriesScoreIndexWhenLoginMarkerHidesCallback()
{
    auto* fakeClient = new FakeCookieHttpClient();
    const QString indexUrl = "http://zhjw.scu.edu.cn/student/integratedQuery/scoreQuery/schemeScores/index";
    const QString callbackUrl = "http://zhjw.scu.edu.cn/student/integratedQuery/scoreQuery/abc/schemeScores/callback";

    HttpResponse loginMarkerResponse;
    loginMarkerResponse.statusCode = 200;
    loginMarkerResponse.finalUrl = QUrl(indexUrl);
    loginMarkerResponse.body = "Login required";

    HttpResponse indexResponse;
    indexResponse.statusCode = 200;
    indexResponse.finalUrl = QUrl(indexUrl);
    indexResponse.body = R"(var url = "/student/integratedQuery/scoreQuery/abc/schemeScores/callback")";
    fakeClient->responseQueues.insert(indexUrl, QList<HttpResponse>{loginMarkerResponse, indexResponse});

    HttpResponse callbackResponse;
    callbackResponse.statusCode = 200;
    callbackResponse.finalUrl = QUrl(callbackUrl);
    callbackResponse.body = R"({"lnList":[]})";
    fakeClient->responses.insert(callbackUrl, callbackResponse);

    FakeZhjwAuthService auth(fakeClient);
    ZhjwApiService service(nullptr, &auth);
    ApiError result;
    service.fetchSchemeScores([&result](const QJsonObject&, const ApiError& error) {
        result = error;
    });

    QCOMPARE(result.type, ApiErrorType::Unknown);
    QCOMPARE(auth.invalidateCount, 1);
    QCOMPARE(fakeClient->getUrls.size(), 3);
    QCOMPARE(fakeClient->getUrls.at(0), indexUrl);
    QCOMPARE(fakeClient->getUrls.at(1), indexUrl);
    QCOMPARE(fakeClient->getUrls.at(2), callbackUrl);
}

// 验证综合教务 API 服务的请求与重试行为。
void PersonBFoundationTest::zhjwApiRetriesOnceWhenSessionExpired()
{
    auto* fakeClient = new FakeCookieHttpClient();
    const QString homeUrl = "http://zhjw.scu.edu.cn/";

    HttpResponse expired;
    expired.statusCode = 200;
    expired.finalUrl = QUrl(homeUrl);
    expired.body = "";

    HttpResponse ok;
    ok.statusCode = 200;
    ok.finalUrl = QUrl(homeUrl);
    ok.body = QString::fromUtf8("<html>当前是第12周</html>").toUtf8();

    fakeClient->responseQueues.insert(homeUrl, {expired, ok});
    FakeZhjwAuthService auth(fakeClient);
    ZhjwApiService service(nullptr, &auth);

    int week = 0;
    ApiError result;
    service.fetchCurrentWeek([&week, &result](int fetchedWeek, const ApiError& error) {
        week = fetchedWeek;
        result = error;
    });

    QCOMPARE(result.type, ApiErrorType::Unknown);
    QCOMPARE(week, 12);
    QCOMPARE(auth.invalidateCount, 1);
    QCOMPARE(fakeClient->getUrls.size(), 2);
}

// 验证综合教务 API 服务不会把普通网络错误误判为会话过期。
void PersonBFoundationTest::zhjwApiPreservesNetworkErrorWithoutSessionRetry()
{
    auto* fakeClient = new FakeCookieHttpClient();
    const QString homeUrl = "http://zhjw.scu.edu.cn/";

    HttpResponse emptyNetworkResponse;
    emptyNetworkResponse.statusCode = 0;
    emptyNetworkResponse.finalUrl = QUrl(homeUrl);
    emptyNetworkResponse.body = "";
    fakeClient->responses.insert(homeUrl, emptyNetworkResponse);

    ApiError networkError;
    networkError.type = ApiErrorType::Network;
    networkError.message = QStringLiteral("connection refused");
    fakeClient->errors.insert(homeUrl, networkError);

    FakeZhjwAuthService auth(fakeClient);
    ZhjwApiService service(nullptr, &auth);

    int week = 0;
    ApiError result;
    service.fetchCurrentWeek([&week, &result](int fetchedWeek, const ApiError& error) {
        week = fetchedWeek;
        result = error;
    });

    QCOMPARE(week, 0);
    QCOMPARE(result.type, ApiErrorType::Network);
    QCOMPARE(result.message, QStringLiteral("connection refused"));
    QCOMPARE(auth.invalidateCount, 0);
    QCOMPARE(fakeClient->getUrls.size(), 1);
}

QTEST_MAIN(PersonBFoundationTest)

#include "test_person_b_foundation.moc"
