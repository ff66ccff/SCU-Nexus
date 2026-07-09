#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

struct AcademicCalendarEntry {
    QString title;
    QString path;

    QVariantMap toVariant() const;
    QJsonObject toJson() const;
    static AcademicCalendarEntry fromJson(const QJsonObject &object);
};

struct AcademicCalendarDetail {
    AcademicCalendarEntry entry;
    QStringList imageUrls;

    QVariantMap toVariant() const;
};

QJsonArray academicCalendarEntriesToJson(const QList<AcademicCalendarEntry> &entries);
QList<AcademicCalendarEntry> academicCalendarEntriesFromJson(const QJsonArray &array);
