#ifndef APIDTOS_H
#define APIDTOS_H

#include <QString>

// B 层只保留服务端学期值和显示文本；“（当前）”标记由课表导入层解释。
struct SemesterDto
{
    QString value;
    QString label;
};

// 教务考表 HTML 的远端解析结果。排序、是否过期、缓存和展示属于查询/ViewModel 层。
struct ExamPlanItemDto
{
    QString courseName;
    QString week;
    QString date;
    QString weekday;
    QString timeRange;
    QString location;
    QString seatNumber;
    QString ticketNumber;
    QString tip = "无";
};

#endif // APIDTOS_H
