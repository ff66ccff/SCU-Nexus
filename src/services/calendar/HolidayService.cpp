#include "services/calendar/HolidayService.h"

// 构造对象并初始化依赖关系。
HolidayService::HolidayService(QObject *parent)
    : QObject(parent)
{
}

// 判断条件是否成立并返回布尔结果。
bool HolidayService::isWeekend(const QDate &date) const
{
    return date.dayOfWeek() == 6 || date.dayOfWeek() == 7;
}
