#include "CookieHttpClient.h"

#include "src/core/logging/AuthLogger.h"
#include "src/core/network/NetworkSettings.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <QTimer>

CookieHttpClient::CookieHttpClient(QObject* parent)
    : QObject(parent)
{
}

// 普通 GET/POST 不自动跟随重定向，避免业务请求悄悄落到统一认证登录页。
void CookieHttpClient::get(const QUrl& url, Callback callback, const Headers& headers)
{
    send(QStringLiteral("GET"), url, {}, std::move(headers), std::move(callback), 0, false);
}

void CookieHttpClient::post(const QUrl& url, const QByteArray& body, Callback callback, const Headers& headers)
{
    send(QStringLiteral("POST"), url, body, std::move(headers), std::move(callback), 0, false);
}

// SSO 专用入口：每一跳都重新按目标 host 计算 Cookie，并保存本跳 Set-Cookie。
void CookieHttpClient::followRedirects(const QUrl& url, Callback callback, const Headers& headers, int maxRedirects)
{
    send(QStringLiteral("GET"), url, {}, std::move(headers), std::move(callback), maxRedirects, false);
}

void CookieHttpClient::clearCookies()
{
    m_cookieJar.clear();
}

QString CookieHttpClient::cookieSummaryForDebug() const
{
    return m_cookieJar.cookieSummaryForDebug();
}

void CookieHttpClient::send(QString method,
                            QUrl url,
                            QByteArray body,
                            Headers headers,
                            Callback callback,
                            int redirectsRemaining,
                            bool retried,
                            int redirectHop)
{
    QNetworkRequest request = buildRequest(url, headers);
    QNetworkReply* reply = nullptr;
    if (method == "POST") {
        reply = m_manager.post(request, body);
    } else {
        reply = m_manager.get(request);
    }

    auto* timeout = new QTimer(reply);
    timeout->setSingleShot(true);
    timeout->setInterval(NetworkSettings::kDefaultTimeoutMs);
    connect(timeout, &QTimer::timeout, reply, [reply]() {
        reply->setProperty("scuTimedOut", true);
        reply->abort();
    });
    timeout->start();

    connect(reply, &QNetworkReply::finished, this, [this,
                                                    reply,
                                                    timeout,
                                                    method,
                                                    url,
                                                    body,
                                                    headers,
                                                    redirectsRemaining,
                                                    retried,
                                                    redirectHop,
                                                    callback = std::move(callback)]() mutable {
        timeout->stop();
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        storeCookies(url, reply);

        if (reply->property("scuTimedOut").toBool()) {
            reply->deleteLater();
            callback({}, networkError(QStringLiteral("网络请求超时"), ApiErrorType::Timeout, statusCode));
            return;
        }

        // Cookie 必须在跳转前保存，否则下一跳拿不到认证站点刚建立的会话。
        const QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        if (statusCode >= 300 && statusCode < 400 && redirectUrl.isValid() && redirectsRemaining > 0) {
            const QUrl nextUrl = url.resolved(redirectUrl);
            // 303 按协议切换成 GET；307/308（以及当前兼容策略下的 301/302）保留方法和 body。
            const bool convertToGet = statusCode == 303;
            AuthLogger::instance().debug(QStringLiteral("CookieHttpClient"),
                                         QStringLiteral("redirect hop=%1 status=%2 %3 -> %4")
                                             .arg(redirectHop)
                                             .arg(statusCode)
                                             .arg(url.host(), nextUrl.host()));
            reply->deleteLater();
            send(convertToGet ? QStringLiteral("GET") : method,
                 nextUrl,
                 convertToGet ? QByteArray() : body,
                 headers,
                 std::move(callback),
                 redirectsRemaining - 1,
                 retried,
                 redirectHop + 1);
            return;
        }
        if (statusCode >= 300 && statusCode < 400 && redirectUrl.isValid() && redirectsRemaining <= 0) {
            AuthLogger::instance().warn(QStringLiteral("CookieHttpClient"),
                                        QStringLiteral("redirect limit exceeded status=%1 host=%2").arg(statusCode).arg(url.host()));
            reply->deleteLater();
            callback({}, networkError(QStringLiteral("SSO 重定向链超过上限"), ApiErrorType::ServiceUnavailable, statusCode));
            return;
        }

        // 有 HTTP 状态码时保留完整响应，例如 rest_token 的 400 body 中有可展示的
        // “验证码错误”。只有连 HTTP 响应都没收到的传输故障才自动重试一次。
        const bool hasHttpResponse = statusCode > 0;
        if (reply->error() != QNetworkReply::NoError && !hasHttpResponse) {
            const QString errorMessage = reply->errorString();
            reply->deleteLater();
            if (!retried) {
                AuthLogger::instance().warn(QStringLiteral("CookieHttpClient"),
                                            QStringLiteral("%1 %2 failed, retrying: %3").arg(method, url.host(), errorMessage));
                send(method, url, body, headers, std::move(callback), redirectsRemaining, true, redirectHop);
                return;
            }
            AuthLogger::instance().error(QStringLiteral("CookieHttpClient"),
                                         QStringLiteral("%1 %2 failed: %3").arg(method, url.host(), errorMessage));
            callback({}, networkError(errorMessage, ApiErrorType::Network, statusCode));
            return;
        }

        HttpResponse response;
        response.statusCode = statusCode;
        response.body = reply->readAll();
        response.finalUrl = url;
        const auto rawPairs = reply->rawHeaderPairs();
        for (const auto& pair : rawPairs) {
            response.headers.insert(QString::fromUtf8(pair.first), QString::fromUtf8(pair.second));
        }
        AuthLogger::instance().debug(QStringLiteral("CookieHttpClient"),
                                     QStringLiteral("%1 %2%3 -> %4")
                                         .arg(method, url.host(), url.path())
                                         .arg(statusCode));
        reply->deleteLater();
        callback(response, {});
    });
}

// 先写默认 UA/Cookie，再应用调用方 headers，允许少数接口覆盖 Accept、Referer 等头。
QNetworkRequest CookieHttpClient::buildRequest(const QUrl& url, const Headers& headers) const
{
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setRawHeader("User-Agent", NetworkSettings::kDefaultUserAgent.toUtf8());

    const QString cookies = m_cookieJar.cookieHeader(url);
    if (!cookies.isEmpty()) {
        request.setRawHeader("Cookie", cookies.toUtf8());
    }

    for (auto it = headers.cbegin(); it != headers.cend(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }
    return request;
}

// Qt 会保留重复的 Set-Cookie 原始头，必须逐项收集，不能从普通 headers map 读取。
void CookieHttpClient::storeCookies(const QUrl& url, QNetworkReply* reply)
{
    QList<QByteArray> setCookieHeaders;
    QStringList cookieNames;
    const auto rawPairs = reply->rawHeaderPairs();
    for (const auto& pair : rawPairs) {
        if (QString::fromUtf8(pair.first).compare("Set-Cookie", Qt::CaseInsensitive) == 0) {
            setCookieHeaders.append(pair.second);
            const QByteArray cookiePair = pair.second.split(';').value(0).trimmed();
            const int equals = cookiePair.indexOf('=');
            if (equals > 0) {
                cookieNames.append(QString::fromUtf8(cookiePair.left(equals)));
            }
        }
    }
    m_cookieJar.storeFromSetCookie(url, setCookieHeaders);
    if (!cookieNames.isEmpty()) {
        AuthLogger::instance().debug(QStringLiteral("CookieHttpClient"),
                                     QStringLiteral("set-cookie host=%1 count=%2 [%3]")
                                         .arg(url.host())
                                         .arg(cookieNames.size())
                                         .arg(cookieNames.join(QLatin1Char(','))));
    }
}

ApiError CookieHttpClient::networkError(const QString& message, ApiErrorType type, int statusCode)
{
    ApiError error;
    error.type = type;
    error.message = message;
    error.statusCode = statusCode;
    return error;
}
