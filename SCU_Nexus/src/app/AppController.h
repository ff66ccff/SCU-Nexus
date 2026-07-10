#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QString>

#include <functional>

class AuthViewModel;
class ScuAuthService;

struct StartupStepResult
{
    bool ok = true;
    QString message;
};

using StartupStep = std::function<StartupStepResult()>;

class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)
    Q_PROPERTY(QString startupError READ startupError NOTIFY startupErrorChanged)
    Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY loggedInChanged)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    Q_PROPERTY(QObject* authViewModel READ authViewModel CONSTANT)
public:
    explicit AppController(QObject *parent = nullptr,
                           ScuAuthService *authService = nullptr,
                           StartupStep queryCacheInitializer = {},
                           StartupStep scheduleInitializer = {});

    bool ready() const;
    QString startupError() const;
    bool loggedIn() const;
    QString appVersion() const;
    QObject* authViewModel() const;

    Q_INVOKABLE void initialize();
    Q_INVOKABLE void setLoginState(bool ok);

signals:
    void readyChanged();
    void startupErrorChanged();
    void loggedInChanged();
    void startupFinished();
    void startupFailed(QString message);
    void loginStateChanged(bool loggedIn);
    void sessionExpired(QString message);
private:
    void setReady(bool ready);
    void setStartupError(const QString& message);
    void failStartup(const QString& message);

    AuthViewModel* m_authViewModel = nullptr;
    StartupStep m_queryCacheInitializer;
    StartupStep m_scheduleInitializer;
    bool m_ready = false;
    QString m_startupError;
    bool m_loggedIn = false;
};

#endif // APPCONTROLLER_H
