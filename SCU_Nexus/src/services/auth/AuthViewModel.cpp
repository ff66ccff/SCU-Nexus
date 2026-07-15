#include "AuthViewModel.h"

#include "ScuAuthService.h"

#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QStandardPaths>

// 默认构造供 QML/应用直接使用；测试和应用装配可通过另一构造函数共享认证服务。
AuthViewModel::AuthViewModel(QObject* parent)
    : QObject(parent)
{
    m_authService = new ScuAuthService(this);
    initializeAuthService();
}

AuthViewModel::AuthViewModel(ScuAuthService* authService, QObject* parent)
    : QObject(parent)
    , m_authService(authService ? authService : new ScuAuthService(this))
{
    initializeAuthService();
}

// ScuAuthService 是认证状态的事实来源；ViewModel 只把信号翻译成 QML 属性和事件。
void AuthViewModel::initializeAuthService()
{
    connect(m_authService, &ScuAuthService::loggedInChanged, this, [this](bool loggedIn) {
        setLoggedIn(loggedIn);
    });
    connect(m_authService, &ScuAuthService::sessionExpired, this, [this](const QString& message) {
        setLoggedIn(false);
        setErrorMessage(message);
        emit sessionExpired(message);
    });
    m_authService->initialize();
    setLoggedIn(m_authService->loggedIn());
}

bool AuthViewModel::loggedIn() const
{
    return m_loggedIn;
}

bool AuthViewModel::loading() const
{
    return m_loading;
}

// 返回验证码图片是否正在加载。
bool AuthViewModel::captchaLoading() const
{
    return m_captchaLoading;
}

// 返回当前验证码图片的本地访问地址。
QUrl AuthViewModel::captchaImageUrl() const
{
    return m_captchaImageUrl;
}

// 返回当前错误提示文本。
QString AuthViewModel::errorMessage() const
{
    return m_errorMessage;
}

// 刷新时先清除旧 code 和旧 URL，防止用户看着新图片却提交上一轮验证码会话。
void AuthViewModel::fetchCaptcha()
{
    clearError();
    m_captchaCode.clear();
    setCaptchaLoading(true);
    setCaptchaImageUrl(QUrl());

    m_authService->fetchCaptcha([this](const CaptchaResult& result, const ApiError& error) {
        setCaptchaLoading(false);
        if (error.type != ApiErrorType::Unknown) {
            m_captchaCode.clear();
            setErrorMessage(error.message);
            return;
        }

        m_captchaCode = result.code;
        setCaptchaImageUrl(writeCaptchaImage(result.imageBytes, result.mimeType));
    });
}

// captchaText 是用户识别出的字符；m_captchaCode 是接口返回的隐藏会话标识。
void AuthViewModel::login(const QString& username, const QString& password, const QString& captchaText)
{
    clearError();
    if (username.trimmed().isEmpty() || password.isEmpty() || captchaText.trimmed().isEmpty()) {
        setErrorMessage(QStringLiteral("请输入账号、密码和验证码"));
        emit loginFailed(m_errorMessage);
        return;
    }
    if (m_captchaCode.trimmed().isEmpty()) {
        setErrorMessage(QStringLiteral("请先刷新验证码"));
        emit loginFailed(m_errorMessage);
        return;
    }

    setLoading(true);
    m_authService->login(username, password, m_captchaCode, captchaText, [this](const ApiError& error) {
        setLoading(false);
        if (error.type != ApiErrorType::Unknown) {
            setErrorMessage(error.message);
            emit loginFailed(error.message);
            return;
        }
        setLoggedIn(true);
        emit loginSucceeded();
    });
}

void AuthViewModel::logout()
{
    m_authService->logout();
    clearError();
}

void AuthViewModel::clearError()
{
    setErrorMessage(QString());
}

// 所有 setter 都只在值实际变化时通知，避免 QML 重复绑定和动画抖动。
void AuthViewModel::setLoggedIn(bool loggedIn)
{
    if (m_loggedIn == loggedIn) {
        return;
    }
    m_loggedIn = loggedIn;
    emit loggedInChanged();
}

void AuthViewModel::setLoading(bool loading)
{
    if (m_loading == loading) {
        return;
    }
    m_loading = loading;
    emit loadingChanged();
}

void AuthViewModel::setCaptchaLoading(bool captchaLoading)
{
    if (m_captchaLoading == captchaLoading) {
        return;
    }
    m_captchaLoading = captchaLoading;
    emit captchaLoadingChanged();
}

void AuthViewModel::setCaptchaImageUrl(const QUrl& url)
{
    if (m_captchaImageUrl == url) {
        return;
    }
    m_captchaImageUrl = url;
    emit captchaChanged();
}

void AuthViewModel::setErrorMessage(const QString& message)
{
    if (m_errorMessage == message) {
        return;
    }
    m_errorMessage = message;
    emit errorChanged();
}

// QML Image 直接消费 file:// URL；递增序号保证即使同一毫秒刷新也不会命中旧缓存。
QUrl AuthViewModel::writeCaptchaImage(const QByteArray& imageBytes, const QString& mimeType)
{
    QString suffix = QStringLiteral("png");
    if (mimeType.contains(QStringLiteral("jpeg")) || mimeType.contains(QStringLiteral("jpg"))) {
        suffix = QStringLiteral("jpg");
    } else if (mimeType.contains(QStringLiteral("gif"))) {
        suffix = QStringLiteral("gif");
    }

    const QString cacheRoot = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir dir(cacheRoot);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    const QString fileName = QStringLiteral("captcha_%1_%2.%3")
        .arg(QDateTime::currentMSecsSinceEpoch())
        .arg(++m_captchaImageSequence)
        .arg(suffix);
    const QString filePath = dir.filePath(fileName);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return {};
    }
    file.write(imageBytes);
    file.close();
    return QUrl::fromLocalFile(filePath);
}
