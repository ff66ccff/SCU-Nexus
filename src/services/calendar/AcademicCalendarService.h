#pragma once

#include "models/AcademicCalendarModels.h"

#include <QNetworkAccessManager>
#include <QObject>

class AcademicCalendarService : public QObject
{
    Q_OBJECT

public:
    explicit AcademicCalendarService(QObject *parent = nullptr);

    Q_INVOKABLE void fetchEntries();
    Q_INVOKABLE void fetchDetail(const AcademicCalendarEntry &entry);

    static QString decodeHtml(const QByteArray &bytes, const QByteArray &contentType = {});
    static QList<AcademicCalendarEntry> parseEntries(const QString &html);
    static QStringList parseImageUrls(const QString &html, const QString &baseUrl = QStringLiteral("https://jwc.scu.edu.cn"));

signals:
    void entriesFetched(const QList<AcademicCalendarEntry> &entries);
    void detailFetched(const AcademicCalendarDetail &detail);
    void failed(const QString &message);

private:
    QNetworkAccessManager m_network;
};
