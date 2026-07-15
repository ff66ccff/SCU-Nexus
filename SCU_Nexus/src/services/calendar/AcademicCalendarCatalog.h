#pragma once

#include "models/AcademicCalendarModels.h"

#include <QList>
#include <QString>

#include <optional>

class AcademicCalendarCatalog
{
public:
    explicit AcademicCalendarCatalog(
        QString source = QStringLiteral(":/calendar/academic_calendars.json"));

    bool load();
    QString errorMessage() const;
    QList<StructuredAcademicCalendar> calendars() const;
    std::optional<StructuredAcademicCalendar> calendarForEntry(
        const QString &title, const QString &path) const;

private:
    QString m_source;
    QString m_errorMessage;
    QList<StructuredAcademicCalendar> m_calendars;
};
