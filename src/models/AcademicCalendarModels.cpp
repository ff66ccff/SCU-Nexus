#include "models/AcademicCalendarModels.h"

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
