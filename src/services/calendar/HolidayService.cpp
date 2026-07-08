#include "services/calendar/HolidayService.h"

HolidayService::HolidayService(QObject *parent)
    : QObject(parent)
{
}

bool HolidayService::isWeekend(const QDate &date) const
{
    return date.dayOfWeek() == 6 || date.dayOfWeek() == 7;
}
