#ifndef AUTHSTATE_H
#define AUTHSTATE_H

#include <QObject>

class AuthEnums : public QObject
{
    Q_OBJECT
public:
    enum class AuthState {
        Unknown,
        Loading,
        Ready,
        Expired,
        Error
    };
    Q_ENUM(AuthState)
};

#endif // AUTHSTATE_H
