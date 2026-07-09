#ifndef ROUTER_H
#define ROUTER_H

#include <QObject>
#include <QString>
#include <QList>

class Router : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentRoute READ currentRoute NOTIFY routeChanged)
    Q_PROPERTY(QString routeTitle READ routeTitle NOTIFY routeChanged)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY routeChanged)

public:
    enum class AppRoute {
        Login,
        Schedule,
        AcademicCalendar,
        ExamPlan,
        Grades,
        Settings
    };
    Q_ENUM(AppRoute)

    explicit Router(QObject *parent = nullptr);

    // 返回当前路由字符串。
    QString currentRoute() const { return m_currentRouteString; }
    QString routeTitle() const;
    // 判断当前状态是否允许执行该操作。
    bool canGoBack() const { return !m_stack.isEmpty(); }

    Q_INVOKABLE void navigate(const QString& route);
    Q_INVOKABLE void replace(const QString& route);
    Q_INVOKABLE void goBack();

signals:
    void routeChanged();

private:
    bool routeFromString(const QString& route, AppRoute* outRoute) const;
    QString routeToString(AppRoute route) const;

    AppRoute m_currentRoute = AppRoute::Schedule;
    QString m_currentRouteString = "Schedule";
    QList<AppRoute> m_stack;
};

#endif // ROUTER_H
