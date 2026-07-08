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
    virtual QString cookieHeaderForDebug() const;

private:
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
    CookieJar m_cookieJar;
};

#endif // COOKIEHTTPCLIENT_H
