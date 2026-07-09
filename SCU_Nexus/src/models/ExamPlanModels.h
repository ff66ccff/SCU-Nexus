#pragma once

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QVariantMap>

struct ExamPlanItem {
    QString courseName;
    QString week;
    QString date;
    QString weekday;
    QString timeRange;
    QString location;
    QString seatNumber;
    QString ticketNumber;
    QString tip;
    bool isPast = false;
    QDateTime startDateTime;
    QDateTime endDateTime;

    QVariantMap toVariant() const;
    QJsonObject toJson() const;
    static ExamPlanItem fromJson(const QJsonObject &object);
};

QDate parseExamDate(const QString &dateText);
QPair<QTime, QTime> parseExamTimeRange(const QString &timeRange);
void updateExamTemporalFields(ExamPlanItem *item, const QDateTime &now = QDateTime::currentDateTime());
void sortExamPlanItems(QList<ExamPlanItem> *items);
QJsonArray examPlanItemsToJson(const QList<ExamPlanItem> &items);
QList<ExamPlanItem> examPlanItemsFromJson(const QJsonArray &array);
