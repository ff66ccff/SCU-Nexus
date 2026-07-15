#ifndef AUTHSTATE_H
#define AUTHSTATE_H

#include <QObject>

// 注册到 Qt 元对象系统的认证状态枚举。QML 日常只消费 loggedIn/loading；
// 这个更完整的状态集留给需要区分“过期”和普通失败的上层组件。
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
