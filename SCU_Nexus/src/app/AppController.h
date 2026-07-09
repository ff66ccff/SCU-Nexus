#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>

class AuthViewModel;
class ScuAuthService;

class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)
    Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY loggedInChanged)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    Q_PROPERTY(QObject* authViewModel READ authViewModel CONSTANT)
public:
    explicit AppController(QObject *parent = nullptr, ScuAuthService *authService = nullptr);

    bool ready() const;
    bool loggedIn() const;
    QString appVersion() const;
    QObject* authViewModel() const;

    Q_INVOKABLE void initialize();
    Q_INVOKABLE void setLoginState(bool ok);

signals:
    void readyChanged();
    void loggedInChanged();
    void startupFinished();
    void startupFailed(QString message);
    void loginStateChanged(bool loggedIn);
    void sessionExpired(QString message);
private:
    void setReady(bool ready);

    AuthViewModel* m_authViewModel = nullptr;
    bool m_ready = false;
    bool m_loggedIn = false;
};

#endif // APPCONTROLLER_H
