#include "AppController.h"

#include "src/services/auth/AuthViewModel.h"

#include <QCoreApplication>

// 构造对象并初始化依赖关系。
AppController::AppController(QObject *parent, ScuAuthService *authService)
    : QObject(parent)
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
