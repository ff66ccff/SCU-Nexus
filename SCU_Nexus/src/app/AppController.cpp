#include "AppController.h"

#include "src/services/auth/AuthViewModel.h"

#include <QCoreApplication>

AppController::AppController(QObject *parent)
    : QObject(parent)
{
    m_authViewModel = new AuthViewModel(this);
    connect(m_authViewModel, &AuthViewModel::loggedInChanged, this, [this]() {
        setLoginState(m_authViewModel->loggedIn());
    });
    connect(m_authViewModel, &AuthViewModel::sessionExpired, this, [this](const QString& message) {
        setLoginState(false);
        emit sessionExpired(message);
    });
}

bool AppController::ready() const
{
    return m_ready;
}

bool AppController::loggedIn() const
{
    return m_loggedIn;
}

QString AppController::appVersion() const
{
    return QCoreApplication::applicationVersion();
}

QObject* AppController::authViewModel() const
{
    return m_authViewModel;
}

void AppController::initialize()
{
    setReady(true);
    emit startupFinished();
}

void AppController::setLoginState(bool ok)
{
    if (m_loggedIn == ok) {
        return;
    }
    m_loggedIn = ok;
    emit loggedInChanged();
    emit loginStateChanged(m_loggedIn);
}

void AppController::setReady(bool ready)
{
    if (m_ready == ready) {
        return;
    }
    m_ready = ready;
    emit readyChanged();
}
