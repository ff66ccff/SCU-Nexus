#include "AppController.h"

#include "src/services/auth/AuthViewModel.h"

#include <QCoreApplication>

#include <utility>

// 构造对象并初始化依赖关系。
AppController::AppController(QObject *parent,
                             ScuAuthService *authService,
                             StartupStep queryCacheInitializer,
                             StartupStep scheduleInitializer)
    : QObject(parent)
    , m_queryCacheInitializer(std::move(queryCacheInitializer))
    , m_scheduleInitializer(std::move(scheduleInitializer))
{
    m_authViewModel = new AuthViewModel(authService, this);
    connect(m_authViewModel, &AuthViewModel::loggedInChanged, this, [this]() {
        setLoginState(m_authViewModel->loggedIn());
    });
    connect(m_authViewModel, &AuthViewModel::sessionExpired, this, [this](const QString& message) {
        setLoginState(false);
        emit sessionExpired(message);
    });
    setLoginState(m_authViewModel->loggedIn());
}

// 返回应用初始化是否已完成。
bool AppController::ready() const
{
    return m_ready;
}

// 返回启动初始化失败时可供界面展示的错误信息。
QString AppController::startupError() const
{
    return m_startupError;
}

// 返回当前登录状态。
bool AppController::loggedIn() const
{
    return m_loggedIn;
}

// 返回当前应用版本号。
QString AppController::appVersion() const
{
    return QCoreApplication::applicationVersion();
}

// 暴露认证视图模型给 QML 层使用。
QObject* AppController::authViewModel() const
{
    return m_authViewModel;
}

// 初始化服务依赖和应用状态。
void AppController::initialize()
{
    if (ready()) {
        return;
    }

    setReady(false);
    setStartupError({});
    if (m_queryCacheInitializer) {
        const StartupStepResult result = m_queryCacheInitializer();
        if (!result.ok) {
            failStartup(QStringLiteral("查询缓存初始化失败：%1").arg(result.message));
            return;
        }
    }
    if (m_scheduleInitializer) {
        const StartupStepResult result = m_scheduleInitializer();
        if (!result.ok) {
            failStartup(QStringLiteral("课表数据初始化失败：%1").arg(result.message));
            return;
        }
    }
    setReady(true);
    emit startupFinished();
}

// 设置属性值并在变化时发出通知。
void AppController::setLoginState(bool ok)
{
    if (m_loggedIn == ok) {
        return;
    }
    m_loggedIn = ok;
    emit loggedInChanged();
    emit loginStateChanged(m_loggedIn);
}

// 设置属性值并在变化时发出通知。
void AppController::setReady(bool ready)
{
    if (m_ready == ready) {
        return;
    }
    m_ready = ready;
    emit readyChanged();
}

// 更新可观察的启动错误信息。
void AppController::setStartupError(const QString& message)
{
    if (m_startupError == message) {
        return;
    }
    m_startupError = message;
    emit startupErrorChanged();
}

// 记录启动失败并通知界面进入可重试错误状态。
void AppController::failStartup(const QString& message)
{
    setStartupError(message);
    emit startupFailed(message);
}
