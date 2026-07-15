#include "services/calendar/AcademicCalendarCatalog.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QUrl>

#include <cmath>
#include <limits>
#include <utility>

namespace {
constexpr auto ScuJwcOrigin = "https://jwc.scu.edu.cn";
const QStringList ExpectedAcademicYearOrder{
    QStringLiteral("2026-2027"),
    QStringLiteral("2025-2026"),
    QStringLiteral("2024-2025"),
    QStringLiteral("2023-2024"),
    QStringLiteral("2022-2023"),
    QStringLiteral("2021-2022")
};

bool fail(QString *error, const QString &message)
{
    *error = message;
    return false;
}

bool requiredString(const QJsonObject &object,
                    const QString &key,
                    const QString &context,
                    QString *result,
                    QString *error)
{
    const QJsonValue value = object.value(key);
    if (!value.isString() || value.toString().trimmed().isEmpty()) {
        return fail(error, QStringLiteral("%1.%2 must be a non-empty string")
                               .arg(context, key));
    }
    *result = value.toString();
    return true;
}

bool requiredArray(const QJsonObject &object,
                   const QString &key,
                   const QString &context,
                   QJsonArray *result,
                   QString *error)
{
    const QJsonValue value = object.value(key);
    if (!value.isArray()) {
        return fail(error, QStringLiteral("%1.%2 must be an array").arg(context, key));
    }
    *result = value.toArray();
    return true;
}

bool requiredInteger(const QJsonObject &object,
                     const QString &key,
                     const QString &context,
                     int *result,
                     QString *error)
{
    const QJsonValue value = object.value(key);
    const double number = value.toDouble(std::numeric_limits<double>::quiet_NaN());
    if (!value.isDouble() || !std::isfinite(number) || number != std::trunc(number)
        || number < std::numeric_limits<int>::min()
        || number > std::numeric_limits<int>::max()) {
        return fail(error, QStringLiteral("%1.%2 must be an integer").arg(context, key));
    }
    *result = static_cast<int>(number);
    return true;
}

bool requiredDate(const QJsonObject &object,
                  const QString &key,
                  const QString &context,
                  QDate *result,
                  QString *error)
{
    QString text;
    if (!requiredString(object, key, context, &text, error)) {
        return false;
    }
    const QDate date = QDate::fromString(text, Qt::ISODate);
    if (!date.isValid() || date.toString(Qt::ISODate) != text) {
        return fail(error, QStringLiteral("%1.%2 must be an ISO date").arg(context, key));
    }
    *result = date;
    return true;
}

bool requiredUrl(const QJsonObject &object,
                 const QString &key,
                 const QString &context,
                 QUrl *result,
                 QString *error)
{
    QString text;
    if (!requiredString(object, key, context, &text, error)) {
        return false;
    }
    const QUrl url(text);
    if (!url.isValid() || url.host().isEmpty()
        || (url.scheme() != QStringLiteral("http") && url.scheme() != QStringLiteral("https"))) {
        return fail(error, QStringLiteral("%1.%2 must be an absolute HTTP URL")
                               .arg(context, key));
    }
    *result = url;
    return true;
}

bool insertUnique(QSet<QString> *values,
                  const QString &value,
                  const QString &context,
                  QString *error)
{
    if (values->contains(value)) {
        return fail(error, QStringLiteral("Duplicate %1: %2").arg(context, value));
    }
    values->insert(value);
    return true;
}

bool parseWeek(const QJsonObject &object,
               const QString &context,
               StructuredCalendarWeek *week,
               QString *error)
{
    static const QSet<QString> phases{
        QStringLiteral("teaching"),
        QStringLiteral("practice"),
        QStringLiteral("winter-break"),
        QStringLiteral("summer-break")
    };

    if (!requiredInteger(object, QStringLiteral("weekNo"), context, &week->weekNo, error)
        || week->weekNo <= 0) {
        if (error->isEmpty()) {
            return fail(error, QStringLiteral("%1.weekNo must be positive").arg(context));
        }
        return false;
    }
    if (!requiredString(object, QStringLiteral("phase"), context, &week->phase, error)
        || !requiredDate(object, QStringLiteral("startDate"), context, &week->startDate, error)
        || !requiredDate(object, QStringLiteral("endDate"), context, &week->endDate, error)
        || !requiredString(object, QStringLiteral("label"), context, &week->label, error)) {
        return false;
    }
    if (!phases.contains(week->phase)) {
        return fail(error, QStringLiteral("%1.phase has unsupported value: %2")
                               .arg(context, week->phase));
    }
    if (week->startDate.daysTo(week->endDate) != 6) {
        return fail(error, QStringLiteral("%1 must span exactly seven days").arg(context));
    }
    return true;
}

bool parseEvent(const QJsonObject &object,
                const QString &context,
                QSet<QString> *eventIds,
                StructuredCalendarEvent *event,
                QString *error)
{
    static const QSet<QString> eventTypes{
        QStringLiteral("registration"),
        QStringLiteral("make-up-exam"),
        QStringLiteral("ceremony"),
        QStringLiteral("instruction"),
        QStringLiteral("holiday"),
        QStringLiteral("athletics"),
        QStringLiteral("final-exam"),
        QStringLiteral("vacation")
    };

    if (!requiredString(object, QStringLiteral("id"), context, &event->id, error)
        || !insertUnique(eventIds, event->id, QStringLiteral("event id"), error)
        || !requiredString(object, QStringLiteral("type"), context, &event->type, error)
        || !requiredString(object, QStringLiteral("title"), context, &event->title, error)
        || !requiredDate(object, QStringLiteral("startDate"), context, &event->startDate, error)
        || !requiredDate(object, QStringLiteral("endDate"), context, &event->endDate, error)) {
        return false;
    }
    if (!eventTypes.contains(event->type)) {
        return fail(error, QStringLiteral("%1.type has unsupported value: %2")
                               .arg(context, event->type));
    }
    if (event->endDate < event->startDate) {
        return fail(error, QStringLiteral("%1 has a reversed date range").arg(context));
    }

    const QJsonValue affectsTeaching = object.value(QStringLiteral("affectsTeaching"));
    if (!affectsTeaching.isUndefined()) {
        if (!affectsTeaching.isBool()) {
            return fail(error, QStringLiteral("%1.affectsTeaching must be a boolean")
                                   .arg(context));
        }
        event->affectsTeaching = affectsTeaching.toBool();
    }
    return true;
}

bool parseNote(const QJsonObject &object,
               const QString &context,
               StructuredCalendarNote *note,
               QString *error)
{
    if (!requiredInteger(object, QStringLiteral("number"), context, &note->order, error)
        || note->order <= 0) {
        if (error->isEmpty()) {
            return fail(error, QStringLiteral("%1.number must be positive").arg(context));
        }
        return false;
    }
    return requiredString(object, QStringLiteral("text"), context, &note->text, error);
}

bool parseTerm(const QJsonObject &object,
               const QString &context,
               QSet<QString> *termIds,
               QSet<QString> *eventIds,
               StructuredCalendarTerm *term,
               QString *error)
{
    if (!requiredString(object, QStringLiteral("id"), context, &term->id, error)
        || !insertUnique(termIds, term->id, QStringLiteral("term id"), error)
        || !requiredString(object, QStringLiteral("name"), context, &term->name, error)
        || !requiredDate(object, QStringLiteral("startDate"), context, &term->startDate, error)
        || !requiredDate(object, QStringLiteral("endDate"), context, &term->endDate, error)
        || !requiredUrl(object, QStringLiteral("sourceImageUrl"), context,
                        &term->sourceImageUrl, error)) {
        return false;
    }
    if (term->endDate < term->startDate) {
        return fail(error, QStringLiteral("%1 has a reversed date range").arg(context));
    }

    QJsonArray weeks;
    QJsonArray events;
    QJsonArray notes;
    if (!requiredArray(object, QStringLiteral("weeks"), context, &weeks, error)
        || !requiredArray(object, QStringLiteral("events"), context, &events, error)
        || !requiredArray(object, QStringLiteral("notes"), context, &notes, error)
        || weeks.isEmpty()) {
        if (error->isEmpty()) {
            return fail(error, QStringLiteral("%1.weeks must not be empty").arg(context));
        }
        return false;
    }

    QSet<int> weekNumbers;
    QDate previousWeekEnd;
    term->weeks.reserve(weeks.size());
    for (qsizetype index = 0; index < weeks.size(); ++index) {
        const QJsonValue value = weeks.at(index);
        if (!value.isObject()) {
            return fail(error, QStringLiteral("%1.weeks[%2] must be an object")
                                   .arg(context).arg(index));
        }
        StructuredCalendarWeek week;
        const QString weekContext = QStringLiteral("%1.weeks[%2]").arg(context).arg(index);
        if (!parseWeek(value.toObject(), weekContext, &week, error)) {
            return false;
        }
        if (weekNumbers.contains(week.weekNo)) {
            return fail(error, QStringLiteral("Duplicate week number in %1: %2")
                                   .arg(context).arg(week.weekNo));
        }
        weekNumbers.insert(week.weekNo);
        if (week.weekNo != index + 1) {
            return fail(error, QStringLiteral("%1 week numbers must be contiguous from 1")
                                   .arg(context));
        }
        if (week.startDate < term->startDate || week.endDate > term->endDate) {
            return fail(error, QStringLiteral("%1 falls outside the term date range")
                                   .arg(weekContext));
        }
        if (previousWeekEnd.isValid() && previousWeekEnd.addDays(1) != week.startDate) {
            return fail(error, QStringLiteral("%1 weeks must have contiguous date ranges")
                                   .arg(context));
        }
        previousWeekEnd = week.endDate;
        term->weeks.append(std::move(week));
    }

    term->events.reserve(events.size());
    for (qsizetype index = 0; index < events.size(); ++index) {
        const QJsonValue value = events.at(index);
        if (!value.isObject()) {
            return fail(error, QStringLiteral("%1.events[%2] must be an object")
                                   .arg(context).arg(index));
        }
        StructuredCalendarEvent event;
        if (!parseEvent(value.toObject(),
                        QStringLiteral("%1.events[%2]").arg(context).arg(index),
                        eventIds, &event, error)) {
            return false;
        }
        term->events.append(std::move(event));
    }

    QSet<int> noteOrders;
    term->notes.reserve(notes.size());
    for (qsizetype index = 0; index < notes.size(); ++index) {
        const QJsonValue value = notes.at(index);
        if (!value.isObject()) {
            return fail(error, QStringLiteral("%1.notes[%2] must be an object")
                                   .arg(context).arg(index));
        }
        StructuredCalendarNote note;
        const QString noteContext = QStringLiteral("%1.notes[%2]").arg(context).arg(index);
        if (!parseNote(value.toObject(), noteContext, &note, error)) {
            return false;
        }
        if (noteOrders.contains(note.order)) {
            return fail(error, QStringLiteral("Duplicate note number in %1: %2")
                                   .arg(context).arg(note.order));
        }
        noteOrders.insert(note.order);
        term->notes.append(std::move(note));
    }
    return true;
}

bool parseCalendar(const QJsonObject &object,
                   const QString &context,
                   QSet<QString> *academicYears,
                   QSet<QString> *termIds,
                   QSet<QString> *eventIds,
                   StructuredAcademicCalendar *calendar,
                   QString *error)
{
    if (!requiredString(object, QStringLiteral("academicYear"), context,
                        &calendar->academicYear, error)
        || !insertUnique(academicYears, calendar->academicYear,
                         QStringLiteral("academic year"), error)
        || !requiredString(object, QStringLiteral("title"), context, &calendar->title, error)
        || !requiredString(object, QStringLiteral("sourcePagePath"), context,
                           &calendar->sourcePagePath, error)
        || !requiredDate(object, QStringLiteral("publishedAt"), context,
                         &calendar->publishedAt, error)) {
        return false;
    }
    static const QRegularExpression academicYearPattern(
        QStringLiteral("^\\d{4}-\\d{4}$"));
    if (!academicYearPattern.match(calendar->academicYear).hasMatch()) {
        return fail(error, QStringLiteral("%1.academicYear has an invalid format").arg(context));
    }
    if (!calendar->sourcePagePath.startsWith(QLatin1Char('/'))) {
        return fail(error, QStringLiteral("%1.sourcePagePath must be an absolute path")
                               .arg(context));
    }

    QJsonArray terms;
    if (!requiredArray(object, QStringLiteral("terms"), context, &terms, error)) {
        return false;
    }
    if (terms.size() != 2) {
        return fail(error, QStringLiteral("%1.terms must contain exactly two terms")
                               .arg(context));
    }
    calendar->terms.reserve(terms.size());
    for (qsizetype index = 0; index < terms.size(); ++index) {
        const QJsonValue value = terms.at(index);
        if (!value.isObject()) {
            return fail(error, QStringLiteral("%1.terms[%2] must be an object")
                                   .arg(context).arg(index));
        }
        StructuredCalendarTerm term;
        if (!parseTerm(value.toObject(),
                       QStringLiteral("%1.terms[%2]").arg(context).arg(index),
                       termIds, eventIds, &term, error)) {
            return false;
        }
        calendar->terms.append(std::move(term));
    }
    return true;
}

QString normalizedPath(QString path)
{
    path = path.trimmed();
    const QString origin = QString::fromLatin1(ScuJwcOrigin);
    if (path.startsWith(origin, Qt::CaseInsensitive)) {
        path.remove(0, origin.size());
    }
    if (!path.isEmpty() && !path.startsWith(QLatin1Char('/'))) {
        path.prepend(QLatin1Char('/'));
    }
    return path;
}

QString academicYearFromTitle(const QString &title)
{
    static const QRegularExpression yearPattern(
        QStringLiteral("(\\d{4})\\s*[-—]\\s*(\\d{4})"));
    const QRegularExpressionMatch match = yearPattern.match(title);
    if (!match.hasMatch()) {
        return {};
    }
    return match.captured(1) + QLatin1Char('-') + match.captured(2);
}
}

AcademicCalendarCatalog::AcademicCalendarCatalog(QString source)
    : m_source(std::move(source))
{
}

bool AcademicCalendarCatalog::load()
{
    m_errorMessage.clear();
    m_calendars.clear();

    QFile file(m_source);
    if (!file.open(QIODevice::ReadOnly)) {
        m_errorMessage = QStringLiteral("Cannot open academic calendar catalog %1: %2")
                             .arg(m_source, file.errorString());
        return false;
    }
    const QByteArray bytes = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        m_errorMessage = QStringLiteral("Cannot read academic calendar catalog %1: %2")
                             .arg(m_source, file.errorString());
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        m_errorMessage = parseError.error == QJsonParseError::NoError
                             ? QStringLiteral("Academic calendar catalog root must be an object")
                             : QStringLiteral("Malformed academic calendar JSON at offset %1: %2")
                                   .arg(parseError.offset)
                                   .arg(parseError.errorString());
        return false;
    }

    const QJsonObject root = document.object();
    int schemaVersion = 0;
    if (!requiredInteger(root, QStringLiteral("schemaVersion"), QStringLiteral("root"),
                         &schemaVersion, &m_errorMessage)) {
        return false;
    }
    if (schemaVersion != 1) {
        m_errorMessage = QStringLiteral("Unsupported academic calendar schema version: %1")
                             .arg(schemaVersion);
        return false;
    }

    QUrl sourceIndexUrl;
    QDate capturedAt;
    QJsonArray calendars;
    if (!requiredUrl(root, QStringLiteral("sourceIndexUrl"), QStringLiteral("root"),
                     &sourceIndexUrl, &m_errorMessage)
        || !requiredDate(root, QStringLiteral("capturedAt"), QStringLiteral("root"),
                         &capturedAt, &m_errorMessage)
        || !requiredArray(root, QStringLiteral("calendars"), QStringLiteral("root"),
                          &calendars, &m_errorMessage)) {
        return false;
    }
    if (calendars.size() != ExpectedAcademicYearOrder.size()) {
        m_errorMessage = QStringLiteral(
            "root.calendars must contain exactly the six captured academic years");
        return false;
    }

    QList<StructuredAcademicCalendar> parsedCalendars;
    parsedCalendars.reserve(calendars.size());
    QSet<QString> academicYears;
    QSet<QString> termIds;
    QSet<QString> eventIds;
    for (qsizetype index = 0; index < calendars.size(); ++index) {
        const QJsonValue value = calendars.at(index);
        if (!value.isObject()) {
            m_errorMessage = QStringLiteral("root.calendars[%1] must be an object").arg(index);
            return false;
        }
        StructuredAcademicCalendar calendar;
        if (!parseCalendar(value.toObject(),
                           QStringLiteral("root.calendars[%1]").arg(index),
                           &academicYears, &termIds, &eventIds, &calendar, &m_errorMessage)) {
            return false;
        }
        if (calendar.academicYear != ExpectedAcademicYearOrder.at(index)) {
            m_errorMessage = QStringLiteral(
                "root.calendars[%1].academicYear must be %2 for the captured baseline")
                                 .arg(index)
                                 .arg(ExpectedAcademicYearOrder.at(index));
            return false;
        }
        parsedCalendars.append(std::move(calendar));
    }

    m_calendars = std::move(parsedCalendars);
    return true;
}

QString AcademicCalendarCatalog::errorMessage() const
{
    return m_errorMessage;
}

QList<StructuredAcademicCalendar> AcademicCalendarCatalog::calendars() const
{
    return m_calendars;
}

std::optional<StructuredAcademicCalendar> AcademicCalendarCatalog::calendarForEntry(
    const QString &title, const QString &path) const
{
    const QString entryPath = normalizedPath(path);
    if (!entryPath.isEmpty()) {
        for (const StructuredAcademicCalendar &calendar : m_calendars) {
            if (normalizedPath(calendar.sourcePagePath) == entryPath) {
                return calendar;
            }
        }
    }

    const QString academicYear = academicYearFromTitle(title);
    if (!academicYear.isEmpty()) {
        for (const StructuredAcademicCalendar &calendar : m_calendars) {
            if (calendar.academicYear == academicYear) {
                return calendar;
            }
        }
    } else if (!title.trimmed().isEmpty()) {
        const QString normalizedTitle = title.trimmed();
        for (const StructuredAcademicCalendar &calendar : m_calendars) {
            if (calendar.title.trimmed() == normalizedTitle) {
                return calendar;
            }
        }
    }
    return std::nullopt;
}
