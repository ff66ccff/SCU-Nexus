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
    // code 是服务端验证码会话标识，不是图片中的字符。
    QString code;
    QByteArray imageBytes;
    QString mimeType;
};

// 四川大学统一认证服务，负责“验证码 -> SM2 登录 -> token -> session/save”。
//
// bindSession 返回的 CookieHttpClient 是后续 SSO 的会话载体并由本服务持有；调用方
// 不得删除它。并发调用会合并成一次 session/save，完成后再广播给全部等待者。
// 当前 token 暂存于 QSettings（不保存密码），这是迁移到系统安全存储前的技术债。
class ScuAuthService : public QObject
{
    Q_OBJECT
public:
    using ClientFactory = std::function<CookieHttpClient*(QObject* parent)>;
    using BindSessionCallback = std::function<void(CookieHttpClient* client, const ApiError& error)>;

    // 构造认证服务并注入设置与 HTTP 客户端工厂。
    explicit ScuAuthService(QObject* parent = nullptr,
                            QSettings* settings = nullptr,
                            ClientFactory clientFactory = {});

    QString accessToken() const;
    qint64 loginTimestamp() const;
    bool loggedIn() const;

    // 时间戳统一使用秒；超过一小时后仍会先尝试 session/save 刷新会话。
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
    // 成功 session/save 后缓存；统一认证、SSO、教务请求必须复用同一实例。
    CookieHttpClient* m_cachedClient = nullptr;
    ClientFactory m_clientFactory;
    // “单飞”状态：首个调用执行网络请求，其余调用只加入回调队列。
    bool m_bindSessionInProgress = false;
    std::vector<BindSessionCallback> m_bindSessionCallbacks;
};

#endif // SCUAUTHSERVICE_H
