#include "ZhjwApiService.h"

#include "src/core/logging/AuthLogger.h"
#include "src/core/network/CookieHttpClient.h"
#include "src/core/network/NetworkSettings.h"
#include "src/services/auth/ZhjwAuthService.h"
#include "src/services/api/ZhjwParsers.h"

#include <QJsonDocument>
#include <QUrlQuery>

namespace {

CookieHttpClient::Headers htmlHeaders(const QString& referer)
{
    return {
        {QStringLiteral("Accept"), QStringLiteral("text/html,*/*")},
        {QStringLiteral("Referer"), referer},
        {QStringLiteral("User-Agent"), NetworkSettings::kDefaultUserAgent},
    };
}

}

ZhjwApiService::ZhjwApiService(QObject* parent, ZhjwAuthService* authService)
    : QObject(parent)
    , m_authService(authService)
{
    if (!m_authService) {
        m_authService = new ZhjwAuthService(this);
        m_ownsAuthService = true;
    }
}

QString ZhjwApiService::scuIdBase()
{
    return QStringLiteral("https://id.scu.edu.cn");
}

QString ZhjwApiService::zhjwBase()
{
    return QStringLiteral("http://zhjw.scu.edu.cn");
}

QString ZhjwApiService::jwcCalendarUrl()
{
    return QStringLiteral("https://jwc.scu.edu.cn/cdxl.htm");
}

bool ZhjwApiService::isSessionExpired(const QString& body, int statusCode)
{
    return ZhjwParsers::isSessionExpired(body, statusCode);
}

void ZhjwApiService::fetchCurrentWeek(WeekCallback callback)
{
    request(QUrl(zhjwBase() + QStringLiteral("/")),
            htmlHeaders(zhjwBase() + QStringLiteral("/")),
            [callback = std::move(callback)](const HttpResponse& response, const ApiError& error) {
        if (error.type != ApiErrorType::Unknown) {
            callback(0, error);
            return;
        }
        const int week = ZhjwParsers::parseCurrentWeek(QString::fromUtf8(response.body));
        if (week <= 0) {
            callback(0, makeError(ApiErrorType::ParseFailed, QStringLiteral("无法获取当前周数，请检查教务系统状态"), response.statusCode, QString::fromUtf8(response.body.left(500))));
            return;
        }
        callback(week, {});
    });
}

void ZhjwApiService::fetchSemesters(SemestersCallback callback)
{
    request(QUrl(zhjwBase() + QStringLiteral("/student/courseSelect/calendarSemesterCurriculum/index")),
            htmlHeaders(zhjwBase() + QStringLiteral("/")),
            [callback = std::move(callback)](const HttpResponse& response, const ApiError& error) {
        if (error.type != ApiErrorType::Unknown) {
            callback({}, error);
            return;
        }
        const QList<SemesterDto> semesters = ZhjwParsers::parseSemesters(QString::fromUtf8(response.body));
        if (semesters.isEmpty()) {
            callback({}, makeError(ApiErrorType::ParseFailed, QStringLiteral("无法获取学期列表，请检查登录状态"), response.statusCode, QString::fromUtf8(response.body.left(500))));
            return;
        }
        callback(semesters, {});
    });
}

void ZhjwApiService::fetchJwxtSchedule(const QString& planCode, ScheduleCallback callback)
{
    QUrlQuery form;
    form.addQueryItem(QStringLiteral("planCode"), planCode);
    postForm(QUrl(zhjwBase() + QStringLiteral("/student/courseSelect/thisSemesterCurriculum/ajaxStudentSchedule/callback")),
             form.toString(QUrl::FullyEncoded).toUtf8(),
             {
                 {QStringLiteral("Accept"), QStringLiteral("application/json, text/javascript, */*; q=0.01")},
                 {QStringLiteral("Content-Type"), QStringLiteral("application/x-www-form-urlencoded; charset=UTF-8")},
                 {QStringLiteral("Referer"), zhjwBase() + QStringLiteral("/student/courseSelect/calendarSemesterCurriculum/index")},
                 {QStringLiteral("User-Agent"), NetworkSettings::kDefaultUserAgent},
                 {QStringLiteral("X-Requested-With"), QStringLiteral("XMLHttpRequest")},
             },
             [callback = std::move(callback)](const HttpResponse& response, const ApiError& error) {
        if (error.type != ApiErrorType::Unknown) {
            callback({}, error);
            return;
        }
        ApiError parseError;
        const QJsonObject json = parseJsonObject(response.body, QStringLiteral("jwxt/schedule"), &parseError);
        callback(json, parseError);
    });
}

void ZhjwApiService::fetchExamPlan(ExamPlanCallback callback)
{
    request(QUrl(zhjwBase() + QStringLiteral("/student/examinationManagement/examPlan/index")),
            htmlHeaders(zhjwBase() + QStringLiteral("/")),
            [callback = std::move(callback)](const HttpResponse& response, const ApiError& error) {
        if (error.type != ApiErrorType::Unknown) {
            callback({}, error);
            return;
        }
        callback(ZhjwParsers::parseExamPlan(QString::fromUtf8(response.body)), {});
    });
}

void ZhjwApiService::fetchSchemeScores(JsonCallback callback)
{
    fetchScoreJson(QStringLiteral("/student/integratedQuery/scoreQuery/schemeScores/index"),
                   QStringLiteral("schemeScores"),
                   QStringLiteral("无法从页面提取 schemeScores callback URL"),
                   std::move(callback));
}

void ZhjwApiService::fetchPassingScores(JsonCallback callback)
{
    fetchScoreJson(QStringLiteral("/student/integratedQuery/scoreQuery/allPassingScores/index"),
                   QStringLiteral("allPassingScores"),
                   QStringLiteral("无法从页面提取 allPassingScores callback URL"),
                   std::move(callback));
}

void ZhjwApiService::request(const QUrl& url,
                             const CookieHttpClient::Headers& headers,
                             ResponseCallback callback,
                             bool retried)
{
    m_authService->getClient([this, url, headers, callback = std::move(callback), retried](CookieHttpClient* client, const ApiError& authError) mutable {
        if (authError.type != ApiErrorType::Unknown || !client) {
            callback({}, authError.type == ApiErrorType::Unknown ? makeError(ApiErrorType::Unauthenticated, QStringLiteral("未登录")) : authError);
            return;
        }
        client->get(url, [this, url, headers, callback = std::move(callback), retried](const HttpResponse& response, const ApiError& error) mutable {
            const QString body = QString::fromUtf8(response.body);
            if ((error.type == ApiErrorType::Unauthenticated || isSessionExpired(body, response.statusCode)) && !retried) {
                AuthLogger::instance().warn(QStringLiteral("ZhjwApiService"),
                                            QStringLiteral("session expired on GET %1, retrying once").arg(url.path()));
                m_authService->invalidate();
                request(url, headers, std::move(callback), true);
                return;
            }
            if (isSessionExpired(body, response.statusCode)) {
                AuthLogger::instance().warn(QStringLiteral("ZhjwApiService"),
                                            QStringLiteral("session expired after retry GET %1").arg(url.path()));
                callback({}, makeError(ApiErrorType::Unauthenticated, QStringLiteral("未登录或登录已过期"), response.statusCode));
                return;
            }
            callback(response, error);
        }, headers);
    });
}

void ZhjwApiService::postForm(const QUrl& url,
                              const QByteArray& body,
                              const CookieHttpClient::Headers& headers,
                              ResponseCallback callback,
                              bool retried)
{
    m_authService->getClient([this, url, body, headers, callback = std::move(callback), retried](CookieHttpClient* client, const ApiError& authError) mutable {
        if (authError.type != ApiErrorType::Unknown || !client) {
            callback({}, authError.type == ApiErrorType::Unknown ? makeError(ApiErrorType::Unauthenticated, QStringLiteral("未登录")) : authError);
            return;
        }
        client->post(url, body, [this, url, body, headers, callback = std::move(callback), retried](const HttpResponse& response, const ApiError& error) mutable {
            const QString responseBody = QString::fromUtf8(response.body);
            if ((error.type == ApiErrorType::Unauthenticated || isSessionExpired(responseBody, response.statusCode)) && !retried) {
                AuthLogger::instance().warn(QStringLiteral("ZhjwApiService"),
                                            QStringLiteral("session expired on POST %1, retrying once").arg(url.path()));
                m_authService->invalidate();
                postForm(url, body, headers, std::move(callback), true);
                return;
            }
            if (isSessionExpired(responseBody, response.statusCode)) {
                AuthLogger::instance().warn(QStringLiteral("ZhjwApiService"),
                                            QStringLiteral("session expired after retry POST %1").arg(url.path()));
                callback({}, makeError(ApiErrorType::Unauthenticated, QStringLiteral("未登录或登录已过期"), response.statusCode));
                return;
            }
            callback(response, error);
        }, headers);
    });
}

void ZhjwApiService::fetchScoreJson(const QString& indexPath,
                                    const QString& callbackKind,
                                    const QString& parseFailureMessage,
                                    JsonCallback callback)
{
    request(QUrl(zhjwBase() + indexPath),
            htmlHeaders(zhjwBase() + QStringLiteral("/")),
            [this, indexPath, callbackKind, parseFailureMessage, callback = std::move(callback)](const HttpResponse& indexResponse, const ApiError& indexError) mutable {
        if (indexError.type != ApiErrorType::Unknown) {
            callback({}, indexError);
            return;
        }
        const QString indexBody = QString::fromUtf8(indexResponse.body);
        const QString callbackPath = callbackKind == QStringLiteral("schemeScores")
            ? ZhjwParsers::extractSchemeScoresCallback(indexBody)
            : ZhjwParsers::extractPassingScoresCallback(indexBody);
        if (callbackPath.isEmpty()) {
            callback({}, makeError(ApiErrorType::ParseFailed, parseFailureMessage, indexResponse.statusCode, indexBody.left(500)));
            return;
        }

        request(QUrl(zhjwBase() + callbackPath),
                {
                    {QStringLiteral("Accept"), QStringLiteral("application/json, text/plain, */*")},
                    {QStringLiteral("Referer"), zhjwBase() + indexPath},
                    {QStringLiteral("User-Agent"), NetworkSettings::kDefaultUserAgent},
                },
                [callback = std::move(callback), callbackKind](const HttpResponse& response, const ApiError& error) {
            if (error.type != ApiErrorType::Unknown) {
                callback({}, error);
                return;
            }
            ApiError parseError;
            const QJsonObject json = parseJsonObject(response.body, callbackKind + QStringLiteral("/callback"), &parseError);
            callback(json, parseError);
        });
    });
}

QJsonObject ZhjwApiService::parseJsonObject(const QByteArray& body, const QString& context, ApiError* error)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        AuthLogger::instance().warn(QStringLiteral("ZhjwApiService"),
                                    QStringLiteral("[%1] JSON parse failed body=%2")
                                        .arg(context, QString::fromUtf8(body.left(500))));
        if (error) {
            *error = makeError(ApiErrorType::ParseFailed,
                               QStringLiteral("[%1] JSON 解析失败").arg(context),
                               0,
                               QString::fromUtf8(body.left(500)));
        }
        return {};
    }
    if (error) {
        *error = {};
    }
    return document.object();
}

ApiError ZhjwApiService::makeError(ApiErrorType type, const QString& message, int statusCode, const QString& debugBody)
{
    ApiError error;
    error.type = type;
    error.message = message;
    error.statusCode = statusCode;
    error.debugBody = debugBody;
    return error;
}
