#include "ZhjwAuthService.h"

#include "ScuAuthService.h"
#include "src/core/logging/AuthLogger.h"
#include "src/core/network/CookieHttpClient.h"
#include "src/core/network/NetworkSettings.h"

namespace {

ApiError makeError(ApiErrorType type, const QString& message, int statusCode = 0)
{
    ApiError error;
    error.type = type;
    error.message = message;
    error.statusCode = statusCode;
    return error;
}

}

ZhjwAuthService::ZhjwAuthService(QObject* parent, ScuAuthService* scuAuthService)
    : QObject(parent)
    , m_scuAuthService(scuAuthService)
{
    if (!m_scuAuthService) {
        m_scuAuthService = new ScuAuthService(this);
        m_scuAuthService->initialize();
        m_ownsScuAuthService = true;
    }
}

// 每次先让 ScuAuthService 校验/重建 session/save，再判断教务 SSO 缓存是否仍可复用。
// 这样缓存命中也不会绕过统一认证的 TTL 和服务端有效性检查。
void ZhjwAuthService::getClient(ClientCallback callback)
{
    m_pendingCallbacks.push_back(std::move(callback));
    if (m_loginInProgress) {
        return;
    }

    m_loginInProgress = true;
    m_scuAuthService->bindSession([this](CookieHttpClient* client, const ApiError& error) mutable {
        if (error.type != ApiErrorType::Unknown || !client) {
            AuthLogger::instance().warn(QStringLiteral("ZhjwAuth"),
                                        QStringLiteral("SCU bindSession failed: %1").arg(error.message));
            finishLogin(nullptr, error.type == ApiErrorType::Unknown ? makeError(ApiErrorType::Unauthenticated, QStringLiteral("未登录")) : error);
            return;
        }

        const QString token = m_scuAuthService->accessToken();
        // client 与 token 必须同时一致；任一变化都意味着原教务 Cookie 不再可信。
        if (m_cachedClient && m_boundAccessToken == token && m_cachedClient == client) {
            AuthLogger::instance().debug(QStringLiteral("ZhjwAuth"), QStringLiteral("getClient cache hit after SCU validation"));
            finishLogin(m_cachedClient, {});
            return;
        }
        if (m_cachedClient) {
            AuthLogger::instance().debug(QStringLiteral("ZhjwAuth"), QStringLiteral("cached client discarded after SCU session change"));
            m_cachedClient = nullptr;
            m_boundAccessToken.clear();
        }

        AuthLogger::instance().info(QStringLiteral("ZhjwAuth"), QStringLiteral("SSO login start"));
        // 必须在 SCU client 本身上走重定向，才能把统一认证 Cookie 逐跳换成教务 Cookie。
        client->followRedirects(QUrl(QStringLiteral("https://id.scu.edu.cn/enduser/sp/sso/scdxplugin_jwt23?enterpriseId=scdx&target_url=index")),
                                [this, client, token](const HttpResponse&, const ApiError& ssoError) mutable {
            if (ssoError.type != ApiErrorType::Unknown) {
                AuthLogger::instance().warn(QStringLiteral("ZhjwAuth"),
                                            QStringLiteral("SSO login failed: %1").arg(ssoError.message));
                finishLogin(nullptr, ssoError);
                return;
            }
            m_cachedClient = client;
            m_boundAccessToken = token;
            AuthLogger::instance().info(QStringLiteral("ZhjwAuth"), QStringLiteral("SSO login ok"));
            finishLogin(m_cachedClient, {});
        }, {
            {QStringLiteral("Accept"), QStringLiteral("text/html,application/xhtml+xml,*/*")},
            {QStringLiteral("Authorization"), QStringLiteral("Bearer %1").arg(token)},
            {QStringLiteral("User-Agent"), NetworkSettings::kDefaultUserAgent},
        }, NetworkSettings::kMaxRedirects);
    });
}

// 与 bindSession 一样先释放“进行中”状态，再广播给所有并发等待者。
void ZhjwAuthService::finishLogin(CookieHttpClient* client, const ApiError& error)
{
    m_loginInProgress = false;
    auto callbacks = std::move(m_pendingCallbacks);
    m_pendingCallbacks.clear();
    for (auto& callback : callbacks) {
        callback(client, error);
    }
}

// 这里只忘记教务绑定，不删除 client：它由 ScuAuthService 持有，重试 SSO 时仍需复用。
void ZhjwAuthService::invalidate()
{
    AuthLogger::instance().debug(QStringLiteral("ZhjwAuth"), QStringLiteral("invalidate"));
    m_cachedClient = nullptr;
    m_boundAccessToken.clear();
    m_loginInProgress = false;
    m_pendingCallbacks.clear();
}
