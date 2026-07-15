#include "ZhjwApiService.h"

#include "src/core/logging/AuthLogger.h"
#include "src/core/network/CookieHttpClient.h"
#include "src/core/network/NetworkSettings.h"
#include "src/services/auth/ZhjwAuthService.h"
#include "src/services/api/ZhjwParsers.h"

#include <QJsonDocument>
#include <QUrlQuery>

namespace {

// 构建教务 HTML 请求所需的通用请求头。
CookieHttpClient::Headers htmlHeaders(const QString& referer)
{
    return {
        {QStringLiteral("Accept"), QStringLiteral("text/html,*/*")},
        {QStringLiteral("Referer"), referer},
        {QStringLiteral("User-Agent"), NetworkSettings::kDefaultUserAgent},
    };
}

// 考表和成绩入口会根据浏览器导航 Accept 返回不同内容，不能复用普通 */* 头。
CookieHttpClient::Headers browserHtmlHeaders(const QString& referer)
{
    return {
        {QStringLiteral("Accept"), QStringLiteral("text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8")},
        {QStringLiteral("Referer"), referer},
        {QStringLiteral("User-Agent"), NetworkSettings::kDefaultUserAgent},
    };
}

// 构造适合诊断的页面摘要，避免原始响应中的认证信息或完整 HTML 泄漏。
QString safeBodySummary(const QString& body)
{
    return AuthLogRedactor::apply(body.simplified()).left(500);
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

// 仅集中暴露公开校历地址；页面抓取和解析由校历模块负责。
QString ZhjwApiService::jwcCalendarUrl()
{
    return QStringLiteral("https://jwc.scu.edu.cn/cdxl.htm");
}

bool ZhjwApiService::isSessionExpired(const QString& body, int statusCode)
{
    return ZhjwParsers::isSessionExpired(body, statusCode);
}

// 首页只负责提供“当前第 N 周”，解析失败不能静默回退为第 1 周。
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

// option 的 value 是后续课表请求 planCode，label 原样交给导入层展示。
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

// 这里只验证响应为 JSON；xkxx 到 Course/ScheduleConfig 的领域转换属于课表模块。
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

// 空列表只有在页面明确出现空结果文案时才算成功，避免把系统维护页伪装成“无考试”。
void ZhjwApiService::fetchExamPlan(ExamPlanCallback callback)
{
    request(QUrl(zhjwBase() + QStringLiteral("/student/examinationManagement/examPlan/index")),
            browserHtmlHeaders(zhjwBase() + QStringLiteral("/")),
            [callback = std::move(callback)](const HttpResponse& response, const ApiError& error) {
        if (error.type != ApiErrorType::Unknown) {
            callback({}, error);
            return;
        }
        const QString body = QString::fromUtf8(response.body);
        const ZhjwParsers::ExamPlanParseResult parseResult = ZhjwParsers::parseExamPlanResult(body);
        if (parseResult.items.isEmpty() && !parseResult.explicitlyEmpty) {
            callback({}, makeError(ApiErrorType::ParseFailed,
                                   QStringLiteral("考试安排页面结构无法识别"),
                                   response.statusCode,
                                   safeBodySummary(body)));
            return;
        }
        callback(parseResult.items, {});
    });
}

// 两种成绩接口共享动态 callback 流程，返回原始 JSON 给成绩模块建模和统计。
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
    // getClient 会合并并发 SSO；拿到的 client 已同时具备统一认证和教务 Cookie。
    m_authService->getClient([this, url, headers, callback = std::move(callback), retried](CookieHttpClient* client, const ApiError& authError) mutable {
        if (authError.type != ApiErrorType::Unknown || !client) {
            callback({}, authError.type == ApiErrorType::Unknown ? makeError(ApiErrorType::Unauthenticated, QStringLiteral("未登录")) : authError);
            return;
        }
        client->get(url, [this, url, headers, callback = std::move(callback), retried](const HttpResponse& response, const ApiError& error) mutable {
            const QString body = QString::fromUtf8(response.body);
            if (error.type != ApiErrorType::Unknown && error.type != ApiErrorType::Unauthenticated) {
                callback(response, error);
                return;
            }
            // 除状态码外还必须识别 200 登录页和空 body，因为教务系统的失效响应并不稳定。
            const bool sessionExpired = isSessionExpired(body, response.statusCode);
            if ((error.type == ApiErrorType::Unauthenticated || sessionExpired) && !retried) {
                AuthLogger::instance().warn(QStringLiteral("ZhjwApiService"),
                                            QStringLiteral("session expired on GET %1, retrying once").arg(url.path()));
                m_authService->invalidate();
                request(url, headers, std::move(callback), true);
                return;
            }
            if (error.type == ApiErrorType::Unauthenticated || sessionExpired) {
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
            if (error.type != ApiErrorType::Unknown && error.type != ApiErrorType::Unauthenticated) {
                callback(response, error);
                return;
            }
            // POST 重试必须保留原 body/headers，课表 planCode 才不会在重新 SSO 后丢失。
            const bool sessionExpired = isSessionExpired(responseBody, response.statusCode);
            if ((error.type == ApiErrorType::Unauthenticated || sessionExpired) && !retried) {
                AuthLogger::instance().warn(QStringLiteral("ZhjwApiService"),
                                            QStringLiteral("session expired on POST %1, retrying once").arg(url.path()));
                m_authService->invalidate();
                postForm(url, body, headers, std::move(callback), true);
                return;
            }
            if (error.type == ApiErrorType::Unauthenticated || sessionExpired) {
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
                                    JsonCallback callback,
                                    bool retried)
{
    request(QUrl(zhjwBase() + indexPath),
            browserHtmlHeaders(zhjwBase() + QStringLiteral("/")),
            [this, indexPath, callbackKind, parseFailureMessage, callback = std::move(callback), retried](const HttpResponse& indexResponse, const ApiError& indexError) mutable {
        if (indexError.type != ApiErrorType::Unknown) {
            callback({}, indexError);
            return;
        }
        const QString indexBody = QString::fromUtf8(indexResponse.body);
        // callback path 含服务端动态片段，每次都从本次入口 HTML 中提取。
        const QString callbackPath = callbackKind == QStringLiteral("schemeScores")
            ? ZhjwParsers::extractSchemeScoresCallback(indexBody)
            : ZhjwParsers::extractPassingScoresCallback(indexBody);
        if (callbackPath.isEmpty()) {
            if (indexBody.contains(QStringLiteral("login"), Qt::CaseInsensitive)) {
                if (!retried) {
                    AuthLogger::instance().warn(QStringLiteral("ZhjwApiService"),
                                                QStringLiteral("score index returned login marker for %1, retrying once").arg(indexPath));
                    m_authService->invalidate();
                    fetchScoreJson(indexPath, callbackKind, parseFailureMessage, std::move(callback), true);
                    return;
                }
                callback({}, makeError(ApiErrorType::Unauthenticated, QStringLiteral("未登录或登录已过期"), indexResponse.statusCode, indexBody.left(500)));
                return;
            }
            callback({}, makeError(ApiErrorType::ParseFailed, parseFailureMessage, indexResponse.statusCode, indexBody.left(500)));
            return;
        }

        // callback 请求的 Referer 必须指回对应入口页，否则教务端可能拒绝或返回 HTML。
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

// B 层只确认顶层为 JSON object，不解释成绩/课表字段，避免与下游领域模型重复维护。
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
