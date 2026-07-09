#include "services/zhjw/MockZhjwApiService.h"

#include <QJsonArray>

// 构造对象并初始化依赖关系。
MockZhjwApiService::MockZhjwApiService(QObject *parent)
    : QObject(parent)
{
}

// 返回当前登录状态。
bool MockZhjwApiService::loggedIn() const
{
    return m_loggedIn;
}

// 设置属性值并在变化时发出通知。
void MockZhjwApiService::setLoggedIn(bool loggedIn)
{
    if (m_loggedIn == loggedIn) {
        return;
    }
    m_loggedIn = loggedIn;
    emit loggedInChanged();
}

// 发起数据获取流程并通过回调返回结果。
QList<QJsonObject> MockZhjwApiService::fetchExamPlan()
{
    return {
        {
            {QStringLiteral("courseName"), QStringLiteral("数据结构")},
            {QStringLiteral("week"), QStringLiteral("第18周")},
            {QStringLiteral("date"), QStringLiteral("2026-01-08")},
            {QStringLiteral("weekday"), QStringLiteral("星期四")},
            {QStringLiteral("timeRange"), QStringLiteral("09:00-11:00")},
            {QStringLiteral("location"), QStringLiteral("江安一教 A101")},
            {QStringLiteral("seatNumber"), QStringLiteral("12")},
            {QStringLiteral("ticketNumber"), QStringLiteral("SCU20260108001")},
            {QStringLiteral("tip"), QStringLiteral("无")}
        },
        {
            {QStringLiteral("courseName"), QStringLiteral("操作系统")},
            {QStringLiteral("week"), QStringLiteral("第19周")},
            {QStringLiteral("date"), QStringLiteral("2026-01-15")},
            {QStringLiteral("weekday"), QStringLiteral("星期四")},
            {QStringLiteral("timeRange"), QStringLiteral("14:00-16:00")},
            {QStringLiteral("location"), QStringLiteral("望江基础教学楼 B203")},
            {QStringLiteral("seatNumber"), QStringLiteral("08")},
            {QStringLiteral("ticketNumber"), QString()},
            {QStringLiteral("tip"), QStringLiteral("携带学生证")}
        }
    };
}

// 发起数据获取流程并通过回调返回结果。
QJsonObject MockZhjwApiService::fetchSchemeScores()
{
    QJsonArray courses = {
        QJsonObject{
            {QStringLiteral("courseName"), QStringLiteral("高等数学")},
            {QStringLiteral("englishCourseName"), QStringLiteral("Advanced Mathematics")},
            {QStringLiteral("courseAttributeName"), QStringLiteral("必修")},
            {QStringLiteral("credit"), QStringLiteral("5")},
            {QStringLiteral("cj"), QStringLiteral("91")},
            {QStringLiteral("courseScore"), 91.0},
            {QStringLiteral("gradePointScore"), 4.0},
            {QStringLiteral("gradeName"), QStringLiteral("A")},
            {QStringLiteral("academicYearCode"), QStringLiteral("2025-2026")},
            {QStringLiteral("termName"), QStringLiteral("秋")}
        },
        QJsonObject{
            {QStringLiteral("courseName"), QStringLiteral("线性代数")},
            {QStringLiteral("englishCourseName"), QStringLiteral("Linear Algebra")},
            {QStringLiteral("courseAttributeName"), QStringLiteral("必修")},
            {QStringLiteral("credit"), QStringLiteral("3")},
            {QStringLiteral("cj"), QStringLiteral("86")},
            {QStringLiteral("courseScore"), 86.0},
            {QStringLiteral("gradePointScore"), 3.6},
            {QStringLiteral("gradeName"), QStringLiteral("B+")},
            {QStringLiteral("academicYearCode"), QStringLiteral("2025-2026")},
            {QStringLiteral("termName"), QStringLiteral("秋")}
        },
        QJsonObject{
            {QStringLiteral("courseName"), QStringLiteral("艺术鉴赏")},
            {QStringLiteral("englishCourseName"), QString()},
            {QStringLiteral("courseAttributeName"), QStringLiteral("任选")},
            {QStringLiteral("credit"), QStringLiteral("2")},
            {QStringLiteral("cj"), QStringLiteral("79")},
            {QStringLiteral("courseScore"), 79.0},
            {QStringLiteral("gradePointScore"), 2.9},
            {QStringLiteral("gradeName"), QStringLiteral("B")},
            {QStringLiteral("academicYearCode"), QStringLiteral("2025-2026")},
            {QStringLiteral("termName"), QStringLiteral("春")}
        }
    };

    return {
        {QStringLiteral("lnList"), QJsonArray{
            QJsonObject{
                {QStringLiteral("zxf"), 160.0},
                {QStringLiteral("yxxf"), 10.0},
                {QStringLiteral("tgms"), 3},
                {QStringLiteral("wtgms"), 0},
                {QStringLiteral("zms"), 3},
                {QStringLiteral("cjlx"), QStringLiteral("方案成绩")},
                {QStringLiteral("cjList"), courses}
            }
        }}
    };
}

// 发起数据获取流程并通过回调返回结果。
QJsonObject MockZhjwApiService::fetchPassingScores()
{
    return {
        {QStringLiteral("lnList"), QJsonArray{
            QJsonObject{
                {QStringLiteral("cjlx"), QStringLiteral("2025-2026学年春(两学期)")},
                {QStringLiteral("cjList"), fetchSchemeScores().value(QStringLiteral("lnList")).toArray().first().toObject().value(QStringLiteral("cjList")).toArray()}
            }
        }}
    };
}
