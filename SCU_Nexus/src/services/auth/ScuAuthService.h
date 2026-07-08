#ifndef SCUAUTHSERVICE_H
#define SCUAUTHSERVICE_H

#include "src/core/network/CookieHttpClient.h"

#include <QByteArray>
#include <QJsonObject>
#include <QObject>
#include <QSettings>
#include <QString>
#include <functional>
#include <vector>

struct CaptchaResult
{
    QString code;
    QByteArray imageBytes;
    QString mimeType;
};

class ScuAuthService : public QObject
{
    Q_OBJECT
public:
    using ClientFactory = std::function<CookieHttpClient*(QObject* parent)>;
    using BindSessionCallback = std::function<void(CookieHttpClient* client, const ApiError& error)>;

    explicit ScuAuthService(QObject* parent = nullptr,
                            QSettings* settings = nullptr,
                            ClientFactory clientFactory = {});

    QString accessToken() const;
    qint64 loginTimestamp() const;
    bool loggedIn() const;

    static bool isTokenExpired(qint64 loginTimestamp, qint64 nowSeconds);
    static CaptchaResult parseCaptchaResponse(const QByteArray& body);

    virtual void initialize();
    virtual void fetchCaptcha(std::function<void(const CaptchaResult& result, const ApiError& error)> callback);
    virtual void login(const QString& username,
                       const QString& password,
                       const QString& captchaCode,
                       const QString& captchaText,
                       std::function<void(const ApiError& error)> callback);
    virtual void bindSession(BindSessionCallback callback);
    virtual void logout();

    void saveTokenForTesting(const QString& token, qint64 loginTimestamp);

signals:
    void loggedInChanged(bool loggedIn);
    void sessionExpired(QString message);

private:
    void saveToken(const QString& token, qint64 loginTimestamp);
    void clearToken();
    void finishBindSession(CookieHttpClient* client, const ApiError& error);

    QSettings* m_settings = nullptr;
    bool m_ownsSettings = false;
    QString m_accessToken;
    qint64 m_loginTimestamp = 0;
    CookieHttpClient* m_cachedClient = nullptr;
    ClientFactory m_clientFactory;
    bool m_bindSessionInProgress = false;
    std::vector<BindSessionCallback> m_bindSessionCallbacks;
};

#endif // SCUAUTHSERVICE_H
