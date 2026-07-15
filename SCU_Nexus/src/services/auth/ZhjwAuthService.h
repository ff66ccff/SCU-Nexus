#ifndef ZHJWAUTHSERVICE_H
#define ZHJWAUTHSERVICE_H

#include "src/core/network/NetworkError.h"

#include <QObject>
#include <QString>
#include <functional>
#include <vector>

class CookieHttpClient;
class ScuAuthService;

// 把统一认证会话升级为教务系统会话。
//
// 它在 ScuAuthService 提供的同一个 CookieHttpClient 上手动走完教务 SSO 重定向链，
// 并按 access token 缓存结果。API 层发现登录页/空响应时调用 invalidate()，下一次
// getClient 会重新执行 SSO。并发请求同样通过等待队列合并。
class ZhjwAuthService : public QObject
{
    Q_OBJECT
public:
    using ClientCallback = std::function<void(CookieHttpClient* client, const ApiError& error)>;

    explicit ZhjwAuthService(QObject* parent = nullptr, ScuAuthService* scuAuthService = nullptr);
    ~ZhjwAuthService() override = default;

    virtual void getClient(ClientCallback callback);

public slots:
    virtual void invalidate();

private:
    void finishLogin(CookieHttpClient* client, const ApiError& error);

    ScuAuthService* m_scuAuthService = nullptr;
    bool m_ownsScuAuthService = false;
    CookieHttpClient* m_cachedClient = nullptr;
    QString m_boundAccessToken;
    bool m_loginInProgress = false;
    std::vector<ClientCallback> m_pendingCallbacks;
};

#endif // ZHJWAUTHSERVICE_H
