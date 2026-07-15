#include "Router.h"

// 构造对象并初始化依赖关系。
Router::Router(QObject *parent) : QObject(parent)
{
    m_currentRoute = AppRoute::Schedule;
    m_currentRouteString = "Schedule";
}

// 在路由枚举和字符串之间转换。
QString Router::routeTitle() const
{
    switch(m_currentRoute) {
    case AppRoute::Login: return "登录";
    case AppRoute::Schedule: return "课表管理";
    case AppRoute::AcademicCalendar: return "校历查询";
    case AppRoute::Classroom: return "教室查询";
    case AppRoute::Grades: return "教务成绩";
    case AppRoute::Settings: return "设置";
    default: return "SCU Nexus";
    }
}

// 导航到指定路由并维护历史栈。
void Router::navigate(const QString& route)
{
    AppRoute newRoute;
    if (!routeFromString(route, &newRoute)) return;

    if (m_currentRoute == newRoute) return;

    m_stack.append(m_currentRoute);
    m_currentRoute = newRoute;
    m_currentRouteString = route;
    emit routeChanged();
}

// 替换当前路由并通知界面切换。
void Router::replace(const QString& route)
{
    AppRoute newRoute;
    if (!routeFromString(route, &newRoute)) return;
    if (m_currentRoute == newRoute) return;

    m_currentRoute = newRoute;
    m_currentRouteString = routeToString(m_currentRoute);
    emit routeChanged();
}

// 返回上一条路由并更新当前页面。
void Router::goBack()
{
    if (!m_stack.isEmpty()) {
        m_currentRoute = m_stack.takeLast();
        m_currentRouteString = routeToString(m_currentRoute);
        emit routeChanged();
    }
}

// 在路由枚举和字符串之间转换。
bool Router::routeFromString(const QString& route, AppRoute* outRoute) const
{
    if (!outRoute) return false;
    if (route == "Login") *outRoute = AppRoute::Login;
    else if (route == "Schedule") *outRoute = AppRoute::Schedule;
    else if (route == "AcademicCalendar") *outRoute = AppRoute::AcademicCalendar;
    else if (route == "Classroom") *outRoute = AppRoute::Classroom;
    else if (route == "Grades") *outRoute = AppRoute::Grades;
    else if (route == "Settings") *outRoute = AppRoute::Settings;
    else return false;
    return true;
}

// 在路由枚举和字符串之间转换。
QString Router::routeToString(AppRoute route) const
{
    switch(route) {
    case AppRoute::Login: return "Login";
    case AppRoute::Schedule: return "Schedule";
    case AppRoute::AcademicCalendar: return "AcademicCalendar";
    case AppRoute::Classroom: return "Classroom";
    case AppRoute::Grades: return "Grades";
    case AppRoute::Settings: return "Settings";
    default: return "";
    }
}
