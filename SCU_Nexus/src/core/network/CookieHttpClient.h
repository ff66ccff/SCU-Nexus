#ifndef COOKIEHTTPCLIENT_H
#define COOKIEHTTPCLIENT_H

#include "src/core/network/CookieJar.h"
#include "src/core/network/HttpResponse.h"
#include "src/core/network/NetworkError.h"

#include <QByteArray>
#include <QMap>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QUrl>

#include <functional>

// 带会话 Cookie 的异步 HTTP 客户端，是统一认证和教务 SSO 共用的会话载体。
//
// 所有回调都由 Qt 网络事件循环触发，不阻塞 UI 线程。HTTP 4xx/5xx 会作为正常
// HttpResponse 返回，只有超时、无 HTTP 响应的网络故障等才写入 ApiError。
// followRedirects 会逐跳保存 Cookie；普通 get/post 禁止跳转并把意外 3xx 作为错误返回。
class CookieHttpClient : public QObject
{
    Q_OBJECT

public:
    using Headers = QMap<QString, QString>;
    using Callback = std::function<void(const HttpResponse& response, const ApiError& error)>;

    explicit CookieHttpClient(QObject* parent = nullptr);
    ~CookieHttpClient() override = default;

    virtual void get(const QUrl& url, Callback callback, const Headers& headers = {});
    virtual void post(const QUrl& url, const QByteArray& body, Callback callback, const Headers& headers = {});
    virtual void followRedirects(const QUrl& url, Callback callback, const Headers& headers = {}, int maxRedirects = 10);

    virtual void clearCookies();
    virtual QString cookieSummaryForDebug() const;

private:
    // redirectsRemaining 为 0 表示不跟随重定向；retried 保证传输故障最多重试一次。
    void send(QString method,
              QUrl url,
              QByteArray body,
              Headers headers,
              Callback callback,
              int redirectsRemaining,
              bool retried,
              int redirectHop = 0);
    QNetworkRequest buildRequest(const QUrl& url, const Headers& headers) const;
    void storeCookies(const QUrl& url, QNetworkReply* reply);
    static ApiError networkError(const QString& message, ApiErrorType type, int statusCode = 0);

    QNetworkAccessManager m_manager;
    // CookieJar 必须与客户端同寿命，SSO 后的教务请求才能复用同一组 Cookie。
    CookieJar m_cookieJar;
};

#endif // COOKIEHTTPCLIENT_H
