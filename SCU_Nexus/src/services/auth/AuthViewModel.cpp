#include "AuthViewModel.h"

#include "ScuAuthService.h"

#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QStandardPaths>

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

bool AuthViewModel::captchaLoading() const
{
    return m_captchaLoading;
}

QUrl AuthViewModel::captchaImageUrl() const
{
    return m_captchaImageUrl;
}

QString AuthViewModel::errorMessage() const
{
    return m_errorMessage;
}

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
