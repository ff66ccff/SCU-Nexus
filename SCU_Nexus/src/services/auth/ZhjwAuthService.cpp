#include "ZhjwAuthService.h"

#include "ScuAuthService.h"
#include "src/core/logging/AuthLogger.h"
#include "src/core/network/CookieHttpClient.h"
#include "src/core/network/NetworkSettings.h"

namespace {

// 构造统一的接口错误对象。
ApiError makeError(ApiErrorType type, const QString& message, int statusCode = 0)
{
    ApiError error;
    error.type = type;
    error.message = message;
    error.statusCode = statusCode;
    return error;
}

}

// 构造对象并初始化依赖关系。
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

// 读取指定资源并返回结果。
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

// 收束异步流程并通知等待中的回调。
void ZhjwAuthService::finishLogin(CookieHttpClient* client, const ApiError& error)
{
    m_loginInProgress = false;
    auto callbacks = std::move(m_pendingCallbacks);
    m_pendingCallbacks.clear();
    for (auto& callback : callbacks) {
        callback(client, error);
    }
}

// 使教务会话缓存失效并释放客户端。
void ZhjwAuthService::invalidate()
{
    AuthLogger::instance().debug(QStringLiteral("ZhjwAuth"), QStringLiteral("invalidate"));
    m_cachedClient = nullptr;
    m_boundAccessToken.clear();
    m_loginInProgress = false;
    m_pendingCallbacks.clear();
}
