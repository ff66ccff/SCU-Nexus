#ifndef APIDTOS_H
#define APIDTOS_H

#include <QString>

struct SemesterDto
{
    QString value;
    QString label;
};

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
