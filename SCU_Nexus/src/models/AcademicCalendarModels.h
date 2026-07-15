#pragma once

#include <QDate>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>
#include <QUrl>
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

struct StructuredCalendarWeek {
    int weekNo = -1;
    QString phase;
    QDate startDate;
    QDate endDate;
    QString label;
    QVariantMap toVariant() const;
};

struct StructuredCalendarEvent {
    QString id;
    QString type;
    QString title;
    QDate startDate;
    QDate endDate;
    bool affectsTeaching = false;
    QVariantMap toVariant() const;
};

struct StructuredCalendarNote {
    int order = 0;
    QString text;
    QVariantMap toVariant() const;
};

struct StructuredCalendarTerm {
    QString id;
    QString name;
    QDate startDate;
    QDate endDate;
    QUrl sourceImageUrl;
    QList<StructuredCalendarWeek> weeks;
    QList<StructuredCalendarEvent> events;
    QList<StructuredCalendarNote> notes;
    QVariantMap toVariant() const;
};

struct StructuredAcademicCalendar {
    QString academicYear;
    QString title;
    QString sourcePagePath;
    QDate publishedAt;
    QList<StructuredCalendarTerm> terms;
    QVariantMap toVariant() const;
};

QJsonArray academicCalendarEntriesToJson(const QList<AcademicCalendarEntry> &entries);
QList<AcademicCalendarEntry> academicCalendarEntriesFromJson(const QJsonArray &array);
