#ifndef APIDTOS_H
#define APIDTOS_H

#include <QList>
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

struct ClassroomCampusDto
{
    QString campusName;
    QString campusNumber;
};

struct ClassroomBuildingDto
{
    QString campusNumber;
    QString teachingBuildingNumber;
    QString teachingBuildingName;
};

struct ClassroomInfoDto
{
    QString classroomName;
    QString classroomStatusCode;
    QString classroomTypeCode;
    QString campusNumber;
    QString classroomNumber;
    QString teachingBuildingNumber;
    int placeNum = 0;
    QString remark;
    QString borrowable;
};

struct ClassroomTimeSlotDto
{
    QString campusNumber;
    QString teachingBuildingNumber;
    QString classroomNumber;
    int weekday = 0;
    int sessionStart = 0;
    int continuingSession = 1;
    QString timeStateNumber;
    QString occupancyModuleId;
};

struct ClassroomIndexDto
{
    QList<ClassroomCampusDto> campuses;
    QList<ClassroomBuildingDto> buildings;
};

struct ClassroomQueryResultDto
{
    QList<ClassroomInfoDto> classrooms;
    QList<ClassroomTimeSlotDto> timeSlots;
    QString date;
    int teachingWeek = 0;
};

#endif // APIDTOS_H
