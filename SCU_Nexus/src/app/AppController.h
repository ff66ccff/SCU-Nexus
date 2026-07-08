#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>

class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY loggedInChanged)
public:
    explicit AppController(QObject *parent = nullptr);
    bool loggedIn() const;
    Q_INVOKABLE void setLoginState(bool ok);

signals:
    void loggedInChanged();
private:
    bool m_loggedIn = false;
};

#endif // APPCONTROLLER_H
