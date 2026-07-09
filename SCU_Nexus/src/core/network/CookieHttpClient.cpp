#include "CookieHttpClient.h"

#include "src/core/logging/AuthLogger.h"
#include "src/core/network/NetworkSettings.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <QTimer>

// 构造对象并初始化依赖关系。
CookieHttpClient::CookieHttpClient(QObject* parent)
    : QObject(parent)
{
}

// 读取指定资源并返回结果。
void CookieHttpClient::get(const QUrl& url, Callback callback, const Headers& headers)
{
    send(QStringLiteral("GET"), url, {}, std::move(headers), std::move(callback), 0, false);
}

// 发送 POST 请求并复用统一网络处理流程。
void CookieHttpClient::post(const QUrl& url, const QByteArray& body, Callback callback, const Headers& headers)
{
    send(QStringLiteral("POST"), url, body, std::move(headers), std::move(callback), 0, false);
}

// 发起请求并按限制自动跟随重定向。
void CookieHttpClient::followRedirects(const QUrl& url, Callback callback, const Headers& headers, int maxRedirects)
{
    send(QStringLiteral("GET"), url, {}, std::move(headers), std::move(callback), maxRedirects, false);
}

// 清理内部状态或持久化数据。
void CookieHttpClient::clearCookies()
{
    m_cookieJar.clear();
}

// 处理 Cookie 的解析、存储或输出。
QString CookieHttpClient::cookieHeaderForDebug() const
{
    return m_cookieJar.cookieHeaderForDebug();
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

        const QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        if (statusCode >= 300 && statusCode < 400 && redirectUrl.isValid() && redirectsRemaining > 0) {
            const QUrl nextUrl = url.resolved(redirectUrl);
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

// 构建携带公共请求头和 Cookie 的网络请求。
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

// 从响应头中提取 Cookie 并保存到 CookieJar。
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

// 构造统一的网络错误对象。
ApiError CookieHttpClient::networkError(const QString& message, ApiErrorType type, int statusCode)
{
    ApiError error;
    error.type = type;
    error.message = message;
    error.statusCode = statusCode;
    return error;
}
