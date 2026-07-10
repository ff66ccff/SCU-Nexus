#include "ScuAuthService.h"

#include "src/core/crypto/Sm2Crypto.h"
#include "src/core/logging/AuthLogger.h"
#include "src/core/network/NetworkSettings.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <memory>

namespace {

constexpr auto kAccessTokenKey = "auth/accessToken";
constexpr auto kLoginTimestampKey = "auth/loginTimestamp";
constexpr qint64 kSessionDurationSeconds = 3600;

// 构造统一的接口错误对象。
ApiError makeError(ApiErrorType type, const QString& message, int statusCode = 0, const QString& debugBody = {})
{
    ApiError error;
    error.type = type;
    error.message = message;
    error.statusCode = statusCode;
    error.debugBody = debugBody;
    return error;
}

// 按候选字段名读取第一个非空 JSON 字符串。
QString jsonString(const QJsonObject& object, std::initializer_list<const char*> keys)
{
    for (const char* key : keys) {
        const QJsonValue value = object.value(QLatin1String(key));
        if (value.isString() && !value.toString().isEmpty()) {
            return value.toString();
        }
    }
    return {};
}

// 判断条件是否成立并返回布尔结果。
bool isCaptchaErrorMessage(const QString& message)
{
    return message.contains(QStringLiteral("验证码"))
        || message.contains(QStringLiteral("captcha"), Qt::CaseInsensitive);
}

}

// 构造对象并初始化依赖关系。
ScuAuthService::ScuAuthService(QObject* parent, QSettings* settings, ClientFactory clientFactory)
    : QObject(parent)
    , m_settings(settings)
    , m_clientFactory(std::move(clientFactory))
{
    if (!m_settings) {
        // TODO(Person B): replace QSettings with OS secure storage before storing production tokens long term.
        m_settings = new QSettings(QStringLiteral("SCU"), QStringLiteral("SCU_Nexus"), this);
        m_ownsSettings = true;
    }
    if (!m_clientFactory) {
        m_clientFactory = [](QObject* parent) {
            return new CookieHttpClient(parent);
        };
    }
}

// 返回当前统一身份认证访问令牌。
QString ScuAuthService::accessToken() const
{
    return m_accessToken;
}

// 返回当前令牌的登录时间戳。
qint64 ScuAuthService::loginTimestamp() const
{
    return m_loginTimestamp;
}

// 返回当前登录状态。
bool ScuAuthService::loggedIn() const
{
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    return !m_accessToken.isEmpty() && !isTokenExpired(m_loginTimestamp, now);
}

// 判断条件是否成立并返回布尔结果。
bool ScuAuthService::isTokenExpired(qint64 loginTimestamp, qint64 nowSeconds)
{
    if (loginTimestamp <= 0) {
        return true;
    }
    return nowSeconds - loginTimestamp > kSessionDurationSeconds;
}

// 解析外部数据并转换为内部结构。
CaptchaResult ScuAuthService::parseCaptchaResponse(const QByteArray& body)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        throw makeError(ApiErrorType::ParseFailed, QStringLiteral("验证码接口 JSON 解析失败"), 0, QString::fromUtf8(body));
    }

    const QJsonObject root = document.object();
    const QJsonObject data = root.value(QStringLiteral("data")).toObject();
    const QString code = data.value(QStringLiteral("code")).toString();
    QString image = jsonString(data, {"captcha", "image", "img", "captchaImage"});
    if (code.isEmpty() || image.isEmpty()) {
        throw makeError(ApiErrorType::ParseFailed, QStringLiteral("验证码字段解析失败"), 0, QString::fromUtf8(body));
    }

    QString mimeType = QStringLiteral("image/png");
    const qsizetype comma = image.indexOf(',');
    if (image.startsWith(QStringLiteral("data:")) && comma > 5) {
        const QString meta = image.mid(5, comma - 5);
        const qsizetype semicolon = meta.indexOf(';');
        mimeType = semicolon >= 0 ? meta.left(semicolon) : meta;
        image = image.mid(comma + 1);
    }

    CaptchaResult result;
    result.code = code;
    result.mimeType = mimeType;
    result.imageBytes = QByteArray::fromBase64(image.toUtf8());
    if (result.imageBytes.isEmpty()) {
        throw makeError(ApiErrorType::ParseFailed, QStringLiteral("验证码图片解析失败"), 0, QString::fromUtf8(body));
    }
    return result;
}

// 初始化服务依赖和应用状态。
void ScuAuthService::initialize()
{
    m_accessToken = m_settings->value(kAccessTokenKey).toString();
    m_loginTimestamp = m_settings->value(kLoginTimestampKey).toLongLong();
    AuthLogger::instance().info(QStringLiteral("ScuAuth"),
                                m_accessToken.isEmpty()
                                    ? QStringLiteral("init: no saved token")
                                    : QStringLiteral("init: token restored ts=%1").arg(m_loginTimestamp));
    emit loggedInChanged(loggedIn());
}

// 发起数据获取流程并通过回调返回结果。
void ScuAuthService::fetchCaptcha(std::function<void(const CaptchaResult& result, const ApiError& error)> callback)
{
    auto* client = m_clientFactory(this);
    const qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    const QUrl url(QStringLiteral("https://id.scu.edu.cn/api/public/bff/v1.2/one_time_login/captcha?_enterprise_id=scdx&timestamp=%1").arg(timestamp));
    client->get(url, [client, callback = std::move(callback)](const HttpResponse& response, const ApiError& error) {
        client->deleteLater();
        if (error.type != ApiErrorType::Unknown) {
            AuthLogger::instance().warn(QStringLiteral("ScuAuth"),
                                        QStringLiteral("fetchCaptcha failed: %1").arg(error.message));
            callback({}, error);
            return;
        }

        try {
            const CaptchaResult result = parseCaptchaResponse(response.body);
            AuthLogger::instance().debug(QStringLiteral("ScuAuth"),
                                         QStringLiteral("fetchCaptcha ok imageBytes=%1").arg(result.imageBytes.size()));
            callback(result, {});
        } catch (const ApiError& parseError) {
            AuthLogger::instance().warn(QStringLiteral("ScuAuth"),
                                        QStringLiteral("fetchCaptcha parse failed: %1").arg(parseError.message));
            callback({}, parseError);
        }
    }, {
        {QStringLiteral("Accept"), QStringLiteral("application/json, text/plain, */*")},
        {QStringLiteral("Referer"), QStringLiteral("https://id.scu.edu.cn/frontend/login")},
        {QStringLiteral("Origin"), QStringLiteral("https://id.scu.edu.cn")},
        {QStringLiteral("User-Agent"), NetworkSettings::kDefaultUserAgent},
    });
}

void ScuAuthService::login(const QString& username,
                           const QString& password,
                           const QString& captchaCode,
                           const QString& captchaText,
                           std::function<void(const ApiError& error)> callback)
{
    if (username.trimmed().isEmpty() || password.isEmpty() || captchaCode.trimmed().isEmpty() || captchaText.trimmed().isEmpty()) {
        AuthLogger::instance().warn(QStringLiteral("ScuAuth"), QStringLiteral("login rejected locally: missing fields"));
        callback(makeError(ApiErrorType::InvalidCredential, QStringLiteral("请输入账号、密码和验证码")));
        return;
    }

    AuthLogger::instance().info(QStringLiteral("ScuAuth"), QStringLiteral("login start"));
    auto* client = m_clientFactory(this);
    auto attempts = std::make_shared<int>(0);
    auto requestSm2 = std::make_shared<std::function<void()>>();
    auto finalCallback = std::make_shared<std::function<void(const ApiError&)>>(std::move(callback));

    const CookieHttpClient::Headers jsonHeaders = {
        {QStringLiteral("Accept"), QStringLiteral("application/json, text/plain, */*")},
        {QStringLiteral("Content-Type"), QStringLiteral("application/json;charset=UTF-8")},
        {QStringLiteral("Origin"), QStringLiteral("https://id.scu.edu.cn")},
        {QStringLiteral("Referer"), QStringLiteral("https://id.scu.edu.cn/frontend/login")},
        {QStringLiteral("User-Agent"), NetworkSettings::kDefaultUserAgent},
    };

    *requestSm2 = [this,
                   client,
                   attempts,
                   requestSm2,
                   finalCallback,
                   jsonHeaders,
                   username,
                   password,
                   captchaCode,
                   captchaText]() mutable {
        ++(*attempts);
        client->post(QUrl(QStringLiteral("https://id.scu.edu.cn/api/public/bff/v1.2/sm2_key")),
                     QByteArrayLiteral("{}"),
                     [this,
                      client,
                      attempts,
                      requestSm2,
                      finalCallback,
                      jsonHeaders,
                      username,
                      password,
                      captchaCode,
                      captchaText](const HttpResponse& sm2Response, const ApiError& sm2Error) mutable {
            QJsonObject sm2Data;
            if (sm2Error.type == ApiErrorType::Unknown) {
                QJsonParseError parseError;
                const QJsonDocument sm2Json = QJsonDocument::fromJson(sm2Response.body, &parseError);
                if (parseError.error == QJsonParseError::NoError && sm2Json.isObject()) {
                    sm2Data = sm2Json.object().value(QStringLiteral("data")).toObject();
                }
            }

            const QString publicKey = sm2Data.value(QStringLiteral("publicKey")).toString();
            const QString sm2Code = sm2Data.value(QStringLiteral("code")).toString();
            if ((sm2Error.type != ApiErrorType::Unknown || publicKey.isEmpty() || sm2Code.isEmpty()) && *attempts < 3) {
                AuthLogger::instance().warn(QStringLiteral("ScuAuth"),
                                            QStringLiteral("sm2_key attempt %1 failed, retrying").arg(*attempts));
                (*requestSm2)();
                return;
            }
            if (sm2Error.type != ApiErrorType::Unknown) {
                client->deleteLater();
                AuthLogger::instance().error(QStringLiteral("ScuAuth"),
                                             QStringLiteral("sm2_key failed: %1").arg(sm2Error.message));
                (*finalCallback)(sm2Error);
                return;
            }
            if (publicKey.isEmpty() || sm2Code.isEmpty()) {
                client->deleteLater();
                AuthLogger::instance().error(QStringLiteral("ScuAuth"), QStringLiteral("sm2_key missing publicKey/code fields"));
                (*finalCallback)(makeError(ApiErrorType::ParseFailed, QStringLiteral("SM2 公钥字段缺失"), sm2Response.statusCode, QString::fromUtf8(sm2Response.body.left(500))));
                return;
            }
            AuthLogger::instance().debug(QStringLiteral("ScuAuth"), QStringLiteral("sm2_key acquired"));

            const QString encryptedPassword = Sm2Crypto::encryptWithBase64Key(password, publicKey);
            if (encryptedPassword.isEmpty()) {
                client->deleteLater();
                AuthLogger::instance().error(QStringLiteral("ScuAuth"), QStringLiteral("SM2 encryption failed"));
                (*finalCallback)(makeError(ApiErrorType::ServiceUnavailable, QStringLiteral("SM2 密码加密失败")));
                return;
            }

            QJsonObject payload;
            payload.insert(QStringLiteral("client_id"), QStringLiteral("1371cbeda563697537f28d99b4744a973uDKtgYqL5B"));
            payload.insert(QStringLiteral("grant_type"), QStringLiteral("password"));
            payload.insert(QStringLiteral("scope"), QStringLiteral("read"));
            payload.insert(QStringLiteral("username"), username);
            payload.insert(QStringLiteral("password"), encryptedPassword);
            payload.insert(QStringLiteral("_enterprise_id"), QStringLiteral("scdx"));
            payload.insert(QStringLiteral("sm2_code"), sm2Code);
            payload.insert(QStringLiteral("cap_code"), captchaCode);
            payload.insert(QStringLiteral("cap_text"), captchaText);

            client->post(QUrl(QStringLiteral("https://id.scu.edu.cn/api/public/bff/v1.2/rest_token")),
                         QJsonDocument(payload).toJson(QJsonDocument::Compact),
                         [this, client, finalCallback](const HttpResponse& tokenResponse, const ApiError& tokenError) mutable {
                client->deleteLater();
                if (tokenError.type != ApiErrorType::Unknown) {
                    AuthLogger::instance().warn(QStringLiteral("ScuAuth"),
                                                QStringLiteral("rest_token failed: %1").arg(tokenError.message));
                    (*finalCallback)(tokenError);
                    return;
                }

                QJsonParseError parseError;
                const QJsonDocument tokenJson = QJsonDocument::fromJson(tokenResponse.body, &parseError);
                if (parseError.error != QJsonParseError::NoError || !tokenJson.isObject()) {
                    (*finalCallback)(makeError(ApiErrorType::ParseFailed, QStringLiteral("Token 响应 JSON 解析失败"), tokenResponse.statusCode, QString::fromUtf8(tokenResponse.body.left(500))));
                    return;
                }
                const QJsonObject root = tokenJson.object();
                if (root.value(QStringLiteral("success")).toBool() != true) {
                    const QString message = jsonString(root, {"message", "msg"});
                    AuthLogger::instance().warn(QStringLiteral("ScuAuth"),
                                                QStringLiteral("login rejected: %1").arg(message.isEmpty() ? QStringLiteral("登录失败") : message));
                    (*finalCallback)(makeError(isCaptchaErrorMessage(message) ? ApiErrorType::InvalidCaptcha : ApiErrorType::InvalidCredential,
                                               message.isEmpty() ? QStringLiteral("登录失败") : message,
                                               tokenResponse.statusCode,
                                               QString::fromUtf8(tokenResponse.body.left(500))));
                    return;
                }

                const QString token = root.value(QStringLiteral("data")).toObject().value(QStringLiteral("access_token")).toString();
                if (token.isEmpty()) {
                    (*finalCallback)(makeError(ApiErrorType::ParseFailed, QStringLiteral("Token 字段缺失"), tokenResponse.statusCode, QString::fromUtf8(tokenResponse.body.left(500))));
                    return;
                }

                if (m_cachedClient) {
                    m_cachedClient->deleteLater();
                    m_cachedClient = nullptr;
                }
                saveToken(token, QDateTime::currentSecsSinceEpoch());
                AuthLogger::instance().info(QStringLiteral("ScuAuth"),
                                            QStringLiteral("token acquired len=%1").arg(token.size()));
                (*finalCallback)({});
            }, jsonHeaders);
        }, jsonHeaders);
    };

    (*requestSm2)();
}

// 绑定教务系统会话并复用登录态。
void ScuAuthService::bindSession(BindSessionCallback callback)
{
    if (m_accessToken.isEmpty()) {
        callback(nullptr, makeError(ApiErrorType::Unauthenticated, QStringLiteral("未登录")));
        return;
    }
    const bool tokenExpired = isTokenExpired(m_loginTimestamp, QDateTime::currentSecsSinceEpoch());
    if (tokenExpired && m_cachedClient) {
        m_cachedClient->deleteLater();
        m_cachedClient = nullptr;
    }
    if (!tokenExpired && m_cachedClient) {
        callback(m_cachedClient, {});
        return;
    }
    m_bindSessionCallbacks.push_back(std::move(callback));
    if (m_bindSessionInProgress) {
        return;
    }

    m_bindSessionInProgress = true;
    auto* client = m_clientFactory(this);
    const QByteArray body = "{}";
    client->post(QUrl(QStringLiteral("https://id.scu.edu.cn/api/bff/v1.2/commons/session/save")),
                 body,
                 [this, client, tokenExpired](const HttpResponse& response, const ApiError& error) {
        if (error.type != ApiErrorType::Unknown) {
            client->deleteLater();
            const ApiError finalError = tokenExpired
                ? makeError(ApiErrorType::SessionExpired, QStringLiteral("登录已过期，请重新登录"), error.statusCode, error.debugBody)
                : error;
            if (tokenExpired) {
                AuthLogger::instance().warn(QStringLiteral("ScuAuth"),
                                            QStringLiteral("bindSession refresh failed: %1").arg(finalError.message));
                emit sessionExpired(finalError.message);
            }
            finishBindSession(nullptr, finalError);
            return;
        }
        const QJsonDocument document = QJsonDocument::fromJson(response.body);
        if (!document.isObject() || document.object().value(QStringLiteral("success")).toBool() != true) {
            client->deleteLater();
            const ApiError finalError = tokenExpired
                ? makeError(ApiErrorType::SessionExpired, QStringLiteral("登录已过期，请重新登录"), response.statusCode, QString::fromUtf8(response.body))
                : makeError(ApiErrorType::ServiceUnavailable, QStringLiteral("session/save 失败"), response.statusCode, QString::fromUtf8(response.body));
            if (tokenExpired) {
                AuthLogger::instance().warn(QStringLiteral("ScuAuth"),
                                            QStringLiteral("bindSession refresh rejected status=%1").arg(response.statusCode));
                emit sessionExpired(finalError.message);
            }
            finishBindSession(nullptr, finalError);
            return;
        }
        m_cachedClient = client;
        if (tokenExpired) {
            saveToken(m_accessToken, QDateTime::currentSecsSinceEpoch());
            AuthLogger::instance().info(QStringLiteral("ScuAuth"), QStringLiteral("bindSession refreshed expired token"));
        }
        AuthLogger::instance().debug(QStringLiteral("ScuAuth"), QStringLiteral("bindSession ok"));
        finishBindSession(m_cachedClient, {});
    }, {
        {QStringLiteral("Accept"), QStringLiteral("application/json, text/plain, */*")},
        {QStringLiteral("Content-Type"), QStringLiteral("application/json;charset=UTF-8")},
        {QStringLiteral("Authorization"), QStringLiteral("Bearer %1").arg(m_accessToken)},
        {QStringLiteral("Origin"), QStringLiteral("https://id.scu.edu.cn")},
        {QStringLiteral("Referer"), QStringLiteral("https://id.scu.edu.cn/frontend/login")},
        {QStringLiteral("User-Agent"), NetworkSettings::kDefaultUserAgent},
    });
}

// 收束异步流程并通知等待中的回调。
void ScuAuthService::finishBindSession(CookieHttpClient* client, const ApiError& error)
{
    m_bindSessionInProgress = false;
    auto callbacks = std::move(m_bindSessionCallbacks);
    m_bindSessionCallbacks.clear();
    for (auto& callback : callbacks) {
        callback(client, error);
    }
}

// 退出登录并清除会话状态。
void ScuAuthService::logout()
{
    AuthLogger::instance().info(QStringLiteral("ScuAuth"), QStringLiteral("logout"));
    clearToken();
    if (m_cachedClient) {
        m_cachedClient->deleteLater();
        m_cachedClient = nullptr;
    }
    m_bindSessionInProgress = false;
    m_bindSessionCallbacks.clear();
    emit loggedInChanged(false);
}

// 为测试场景写入指定令牌和时间戳。
void ScuAuthService::saveTokenForTesting(const QString& token, qint64 loginTimestamp)
{
    saveToken(token, loginTimestamp);
}

// 保存访问令牌并持久化登录时间戳。
void ScuAuthService::saveToken(const QString& token, qint64 loginTimestamp)
{
    m_accessToken = token;
    m_loginTimestamp = loginTimestamp;
    m_settings->setValue(kAccessTokenKey, m_accessToken);
    m_settings->setValue(kLoginTimestampKey, m_loginTimestamp);
    m_settings->sync();
    emit loggedInChanged(loggedIn());
}

// 清理内部状态或持久化数据。
void ScuAuthService::clearToken()
{
    m_accessToken.clear();
    m_loginTimestamp = 0;
    m_settings->remove(kAccessTokenKey);
    m_settings->remove(kLoginTimestampKey);
    m_settings->sync();
}
