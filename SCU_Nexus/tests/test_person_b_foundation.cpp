#include <QtTest/QtTest>

#include "src/core/network/CookieJar.h"
#include "src/core/crypto/Sm2Crypto.h"
#include "src/core/logging/AuthLogger.h"
#include <QObject>
#include <QUrl>
#define private public
#include "src/services/auth/AuthViewModel.h"
#undef private
#include "src/services/auth/ScuAuthService.h"
#include "src/services/auth/ZhjwAuthService.h"
#include "src/services/api/ZhjwParsers.h"
#include "src/services/api/ZhjwApiService.h"

#include <QEventLoop>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <algorithm>

namespace {

class FakeCookieHttpClient : public CookieHttpClient
{
public:
    explicit FakeCookieHttpClient(QObject* parent = nullptr)
        : CookieHttpClient(parent)
    {
    }

    void get(const QUrl& url, Callback callback, const Headers& headers = {}) override
    {
        Q_UNUSED(headers)
        getUrls.append(url.toString());
        callback(nextResponse(url), {});
    }

    void post(const QUrl& url, const QByteArray& body, Callback callback, const Headers& headers = {}) override
    {
        Q_UNUSED(headers)
        postUrls.append(url.toString());
        postBodies.append(body);
        callback(nextResponse(url), {});
    }

    void followRedirects(const QUrl& url, Callback callback, const Headers& headers = {}, int maxRedirects = 10) override
    {
        Q_UNUSED(headers)
        Q_UNUSED(maxRedirects)
        redirectUrls.append(url.toString());
        callback(nextResponse(url), {});
    }

    QStringList getUrls;
    QStringList postUrls;
    QStringList redirectUrls;
    QList<QByteArray> postBodies;
    QMap<QString, HttpResponse> responses;
    QMap<QString, QList<HttpResponse>> responseQueues;

private:
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

class FakeScuAuthService : public ScuAuthService
{
public:
    explicit FakeScuAuthService(QObject* parent = nullptr)
        : ScuAuthService(parent)
    {
    }

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
    void cookieJarMatchesExactAndParentDomain();
    void cookieJarRejectsUnrelatedDomain();
    void cookieJarParsesCommaSeparatedSetCookie();
    void cookieHttpClientReturnsHttpErrorResponsesWithBody();
    void cookieHttpClientReportsRedirectLimit();
    void cookieHttpClientSendsCookiesStoredDuringRedirect();
    void authLoggerRedactsSensitiveValues();
    void authLoggerCapsRingBuffer();
    void zhjwParsersExtractRequiredValues();
    void zhjwParsersDetectExpiredSessions();
    void authViewModelWritesFreshCaptchaUrlEachTime();
    void authViewModelRejectsLoginBeforeCaptchaLoaded();
    void authViewModelDoesNotEmitLoginFailedForCaptchaFetchErrors();
    void authViewModelForwardsSessionExpired();
    void scuAuthParsesCaptchaDataUrl();
    void scuAuthTokenTtlAndStorageWork();
    void zhjwApiExposesDocumentedEndpoints();
    void sm2CryptoEncryptsToC1C2C3Base64();
    void scuAuthLoginRequestsSm2ThenTokenAndStoresToken();
    void scuAuthLoginParsesRestTokenHttp400JsonError();
    void scuAuthBindSessionCoalescesConcurrentCalls();
    void scuAuthBindSessionRefreshesExpiredTokenWhenSessionSaveSucceeds();
    void zhjwAuthCoalescesConcurrentSsoLogin();
    void zhjwAuthRebindsWhenScuTokenChanges();
    void zhjwApiRetriesOnceWhenSessionExpired();
};

void PersonBFoundationTest::cookieJarMatchesExactAndParentDomain()
{
    CookieJar jar;
    jar.storeFromSetCookie(QUrl("https://id.scu.edu.cn/login"),
                           {QByteArray("SESSION=abc; Path=/; HttpOnly")});

    QCOMPARE(jar.cookieHeader(QUrl("https://id.scu.edu.cn/profile")), QString("SESSION=abc"));
    QCOMPARE(jar.cookieHeader(QUrl("https://sub.id.scu.edu.cn/profile")), QString("SESSION=abc"));
}

void PersonBFoundationTest::cookieJarRejectsUnrelatedDomain()
{
    CookieJar jar;
    jar.storeFromSetCookie(QUrl("https://id.scu.edu.cn/login"),
                           {QByteArray("SESSION=abc; Path=/; HttpOnly")});

    QCOMPARE(jar.cookieHeader(QUrl("https://zhjw.scu.edu.cn/")), QString());
}

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
}

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
          <div>考试提示信息:&nbsp;携带证件</div>
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

void PersonBFoundationTest::zhjwParsersDetectExpiredSessions()
{
    QVERIFY(ZhjwParsers::isSessionExpired("", 200));
    QVERIFY(ZhjwParsers::isSessionExpired("<html>login</html>", 200));
    QVERIFY(ZhjwParsers::isSessionExpired("<html>统一身份认证 用户登录</html>", 200));
    QVERIFY(ZhjwParsers::isSessionExpired("redirect", 302));
    QVERIFY(!ZhjwParsers::isSessionExpired("{\"success\":true}", 200));
}

void PersonBFoundationTest::authViewModelWritesFreshCaptchaUrlEachTime()
{
    AuthViewModel model;

    const QUrl first = model.writeCaptchaImage(QByteArray("first-image"), QStringLiteral("image/png"));
    const QUrl second = model.writeCaptchaImage(QByteArray("second-image"), QStringLiteral("image/png"));

    QVERIFY(first.isLocalFile());
    QVERIFY(second.isLocalFile());
    QVERIFY(first != second);
}

void PersonBFoundationTest::authViewModelRejectsLoginBeforeCaptchaLoaded()
{
    AuthViewModel model;

    model.login(QStringLiteral("20260001"), QStringLiteral("secret"), QStringLiteral("abcd"));

    QCOMPARE(model.loading(), false);
    QCOMPARE(model.errorMessage(), QStringLiteral("请先刷新验证码"));
}

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

void PersonBFoundationTest::zhjwApiExposesDocumentedEndpoints()
{
    QCOMPARE(ZhjwApiService::scuIdBase(), QString("https://id.scu.edu.cn"));
    QCOMPARE(ZhjwApiService::zhjwBase(), QString("http://zhjw.scu.edu.cn"));
    QCOMPARE(ZhjwApiService::jwcCalendarUrl(), QString("https://jwc.scu.edu.cn/cdxl.htm"));
    QVERIFY(ZhjwApiService::isSessionExpired("<html>统一身份认证 用户登录</html>", 200));
}

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
}

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

QTEST_MAIN(PersonBFoundationTest)

#include "test_person_b_foundation.moc"
