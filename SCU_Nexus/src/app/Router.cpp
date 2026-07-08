#include "Router.h"

Router::Router(QObject *parent) : QObject(parent)
{
    m_currentRoute = AppRoute::Schedule;
    m_currentRouteString = "Schedule";
}

QString Router::routeTitle() const
{
    switch(m_currentRoute) {
    case AppRoute::Login: return "登录";
    case AppRoute::Schedule: return "课表管理";
    case AppRoute::AcademicCalendar: return "校历查询";
    case AppRoute::ExamPlan: return "考表查询";
    case AppRoute::Grades: return "教务成绩";
    case AppRoute::Settings: return "设置";
    default: return "SCU Nexus";
    }
}

void Router::navigate(const QString& route)
{
    AppRoute newRoute;
    if (route == "Login") newRoute = AppRoute::Login;
    else if (route == "Schedule") newRoute = AppRoute::Schedule;
    else if (route == "AcademicCalendar") newRoute = AppRoute::AcademicCalendar;
    else if (route == "ExamPlan") newRoute = AppRoute::ExamPlan;
    else if (route == "Grades") newRoute = AppRoute::Grades;
    else if (route == "Settings") newRoute = AppRoute::Settings;
    else return;

    if (m_currentRoute == newRoute) return;

    m_stack.append(m_currentRoute);
    m_currentRoute = newRoute;
    m_currentRouteString = route;
    emit routeChanged();
}

void Router::goBack()
{
    if (!m_stack.isEmpty()) {
        m_currentRoute = m_stack.takeLast();
        m_currentRouteString = routeToString(m_currentRoute);
        emit routeChanged();
    }
}

QString Router::routeToString(AppRoute route) const
{
    switch(route) {
    case AppRoute::Login: return "Login";
    case AppRoute::Schedule: return "Schedule";
    case AppRoute::AcademicCalendar: return "AcademicCalendar";
    case AppRoute::ExamPlan: return "ExamPlan";
    case AppRoute::Grades: return "Grades";
    case AppRoute::Settings: return "Settings";
    default: return "";
    }
}