#include "models/AcademicCalendarModels.h"

QVariantMap AcademicCalendarEntry::toVariant() const
{
    return {
        {QStringLiteral("title"), title},
        {QStringLiteral("path"), path}
    };
}

QJsonObject AcademicCalendarEntry::toJson() const
{
    return {
        {QStringLiteral("title"), title},
        {QStringLiteral("path"), path}
    };
}

AcademicCalendarEntry AcademicCalendarEntry::fromJson(const QJsonObject &object)
{
    return {
        object.value(QStringLiteral("title")).toString(),
        object.value(QStringLiteral("path")).toString()
    };
}

QVariantMap AcademicCalendarDetail::toVariant() const
{
    return {
        {QStringLiteral("entry"), entry.toVariant()},
        {QStringLiteral("imageUrls"), imageUrls}
    };
}

QJsonArray academicCalendarEntriesToJson(const QList<AcademicCalendarEntry> &entries)
{
    QJsonArray array;
    for (const AcademicCalendarEntry &entry : entries) {
        array.append(entry.toJson());
    }
    return array;
}

QList<AcademicCalendarEntry> academicCalendarEntriesFromJson(const QJsonArray &array)
{
    QList<AcademicCalendarEntry> entries;
    for (const QJsonValue &value : array) {
        if (value.isObject()) {
            entries.append(AcademicCalendarEntry::fromJson(value.toObject()));
        }
    }
    return entries;
}
