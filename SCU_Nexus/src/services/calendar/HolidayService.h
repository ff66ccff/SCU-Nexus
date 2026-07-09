#pragma once

#include <QDate>
#include <QObject>

class HolidayService : public QObject
{
    Q_OBJECT

public:
    explicit HolidayService(QObject *parent = nullptr);

    Q_INVOKABLE bool isWeekend(const QDate &date) const;
};
