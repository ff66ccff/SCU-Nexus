#include "models/AcademicCalendarModels.h"

namespace {
template<typename T>
QVariantList toVariantList(const QList<T> &items)
{
    QVariantList values;
    values.reserve(items.size());
    for (const T &item : items) {
        values.append(item.toVariant());
    }
    return values;
}
}

// 转换为界面可直接读取的 QVariant 数据。
QVariantMap AcademicCalendarEntry::toVariant() const
{
    return {
        {QStringLiteral("title"), title},
        {QStringLiteral("path"), path}
    };
}

// 序列化为 JSON 对象，便于缓存或传输。
QJsonObject AcademicCalendarEntry::toJson() const
{
    return {
        {QStringLiteral("title"), title},
        {QStringLiteral("path"), path}
    };
}

// 从 JSON 对象还原数据模型。
AcademicCalendarEntry AcademicCalendarEntry::fromJson(const QJsonObject &object)
{
    return {
        object.value(QStringLiteral("title")).toString(),
        object.value(QStringLiteral("path")).toString()
    };
}

// 转换为界面可直接读取的 QVariant 数据。
QVariantMap AcademicCalendarDetail::toVariant() const
{
    return {
        {QStringLiteral("entry"), entry.toVariant()},
        {QStringLiteral("imageUrls"), imageUrls}
    };
}

QVariantMap StructuredCalendarWeek::toVariant() const
{
    return {
        {QStringLiteral("weekNo"), weekNo},
        {QStringLiteral("phase"), phase},
        {QStringLiteral("startDate"), startDate},
        {QStringLiteral("endDate"), endDate},
        {QStringLiteral("label"), label}
    };
}

QVariantMap StructuredCalendarEvent::toVariant() const
{
    return {
        {QStringLiteral("id"), id},
        {QStringLiteral("type"), type},
        {QStringLiteral("title"), title},
        {QStringLiteral("startDate"), startDate},
        {QStringLiteral("endDate"), endDate},
        {QStringLiteral("affectsTeaching"), affectsTeaching}
    };
}

QVariantMap StructuredCalendarNote::toVariant() const
{
    return {
        {QStringLiteral("order"), order},
        {QStringLiteral("text"), text}
    };
}

QVariantMap StructuredCalendarTerm::toVariant() const
{
    return {
        {QStringLiteral("id"), id},
        {QStringLiteral("name"), name},
        {QStringLiteral("startDate"), startDate},
        {QStringLiteral("endDate"), endDate},
        {QStringLiteral("sourceImageUrl"), sourceImageUrl},
        {QStringLiteral("weeks"), toVariantList(weeks)},
        {QStringLiteral("events"), toVariantList(events)},
        {QStringLiteral("notes"), toVariantList(notes)}
    };
}

QVariantMap StructuredAcademicCalendar::toVariant() const
{
    return {
        {QStringLiteral("academicYear"), academicYear},
        {QStringLiteral("title"), title},
        {QStringLiteral("sourcePagePath"), sourcePagePath},
        {QStringLiteral("publishedAt"), publishedAt},
        {QStringLiteral("terms"), toVariantList(terms)}
    };
}

// 将校历条目列表序列化为 JSON 数组。
QJsonArray academicCalendarEntriesToJson(const QList<AcademicCalendarEntry> &entries)
{
    QJsonArray array;
    for (const AcademicCalendarEntry &entry : entries) {
        array.append(entry.toJson());
    }
    return array;
}

// 从 JSON 数组还原校历条目列表。
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
