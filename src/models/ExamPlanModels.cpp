#include "models/ExamPlanModels.h"

#include <algorithm>
#include <QRegularExpression>

QVariantMap ExamPlanItem::toVariant() const
{
    return {
        {QStringLiteral("courseName"), courseName},
        {QStringLiteral("week"), week},
        {QStringLiteral("date"), date},
        {QStringLiteral("weekday"), weekday},
        {QStringLiteral("timeRange"), timeRange},
        {QStringLiteral("location"), location},
        {QStringLiteral("seatNumber"), seatNumber},
        {QStringLiteral("ticketNumber"), ticketNumber},
        {QStringLiteral("tip"), tip},
        {QStringLiteral("isPast"), isPast},
        {QStringLiteral("startDateTime"), startDateTime},
        {QStringLiteral("endDateTime"), endDateTime}
    };
}

QJsonObject ExamPlanItem::toJson() const
{
    return {
        {QStringLiteral("courseName"), courseName},
        {QStringLiteral("week"), week},
        {QStringLiteral("date"), date},
        {QStringLiteral("weekday"), weekday},
        {QStringLiteral("timeRange"), timeRange},
        {QStringLiteral("location"), location},
        {QStringLiteral("seatNumber"), seatNumber},
        {QStringLiteral("ticketNumber"), ticketNumber},
        {QStringLiteral("tip"), tip}
    };
}

ExamPlanItem ExamPlanItem::fromJson(const QJsonObject &object)
{
    ExamPlanItem item;
    item.courseName = object.value(QStringLiteral("courseName")).toString();
    item.week = object.value(QStringLiteral("week")).toString();
    item.date = object.value(QStringLiteral("date")).toString();
    item.weekday = object.value(QStringLiteral("weekday")).toString();
    item.timeRange = object.value(QStringLiteral("timeRange")).toString();
    item.location = object.value(QStringLiteral("location")).toString();
    item.seatNumber = object.value(QStringLiteral("seatNumber")).toString();
    item.ticketNumber = object.value(QStringLiteral("ticketNumber")).toString();
    item.tip = object.value(QStringLiteral("tip")).toString();
    updateExamTemporalFields(&item);
    return item;
}

QDate parseExamDate(const QString &dateText)
{
    const QString trimmed = dateText.trimmed();
    const QList<QString> formats = {
        QStringLiteral("yyyy-MM-dd"),
        QStringLiteral("yyyy/M/d"),
        QStringLiteral("yyyy/M/dd"),
        QStringLiteral("yyyy年M月d日")
    };
    for (const QString &format : formats) {
        const QDate date = QDate::fromString(trimmed, format);
        if (date.isValid()) {
            return date;
        }
    }

    static const QRegularExpression re(QStringLiteral("(\\d{4})[-/年](\\d{1,2})[-/月](\\d{1,2})"));
    const QRegularExpressionMatch match = re.match(trimmed);
    if (match.hasMatch()) {
        return QDate(match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt());
    }
    return {};
}

QPair<QTime, QTime> parseExamTimeRange(const QString &timeRange)
{
    static const QRegularExpression re(QStringLiteral("(\\d{1,2}:\\d{2})\\s*[-~至]\\s*(\\d{1,2}:\\d{2})"));
    const QRegularExpressionMatch match = re.match(timeRange);
    if (!match.hasMatch()) {
        return {};
    }
    const QTime start = QTime::fromString(match.captured(1), QStringLiteral("H:mm"));
    const QTime end = QTime::fromString(match.captured(2), QStringLiteral("H:mm"));
    return {start, end};
}

void updateExamTemporalFields(ExamPlanItem *item, const QDateTime &now)
{
    if (!item) {
        return;
    }

    const QDate date = parseExamDate(item->date);
    const auto range = parseExamTimeRange(item->timeRange);
    if (date.isValid() && range.first.isValid() && range.second.isValid()) {
        item->startDateTime = QDateTime(date, range.first);
        item->endDateTime = QDateTime(date, range.second);
        item->isPast = now > item->endDateTime;
        return;
    }

    if (date.isValid()) {
        item->isPast = now.date() > date.addDays(1);
    } else {
        item->isPast = false;
    }
}

void sortExamPlanItems(QList<ExamPlanItem> *items)
{
    if (!items) {
        return;
    }

    std::sort(items->begin(), items->end(), [](const ExamPlanItem &left, const ExamPlanItem &right) {
        const bool leftHasTime = left.startDateTime.isValid();
        const bool rightHasTime = right.startDateTime.isValid();
        if (leftHasTime != rightHasTime) {
            return leftHasTime;
        }
        if (leftHasTime && left.startDateTime != right.startDateTime) {
            return left.startDateTime < right.startDateTime;
        }
        return left.courseName.localeAwareCompare(right.courseName) < 0;
    });
}

QJsonArray examPlanItemsToJson(const QList<ExamPlanItem> &items)
{
    QJsonArray array;
    for (const ExamPlanItem &item : items) {
        array.append(item.toJson());
    }
    return array;
}

QList<ExamPlanItem> examPlanItemsFromJson(const QJsonArray &array)
{
    QList<ExamPlanItem> items;
    for (const QJsonValue &value : array) {
        if (value.isObject()) {
            items.append(ExamPlanItem::fromJson(value.toObject()));
        }
    }
    sortExamPlanItems(&items);
    return items;
}
