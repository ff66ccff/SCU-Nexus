#include "AppController.h"

AppController::AppController(QObject *parent)
    : QObject(parent)
{

}

bool AppController::loggedIn() const
{
    return m_loggedIn;
}

void AppController::setLoginState(bool ok)
{
    m_loggedIn = ok;
    emit loggedInChanged();
}
