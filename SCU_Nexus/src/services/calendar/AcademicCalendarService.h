#pragma once

#include "core/network/NetworkError.h"
#include "models/AcademicCalendarModels.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QObject>
#include <QMetaType>

Q_DECLARE_METATYPE(ApiError)

class AcademicCalendarService : public QObject
{
    Q_OBJECT

public:
    explicit AcademicCalendarService(QObject *parent = nullptr);
    explicit AcademicCalendarService(QNetworkAccessManager *network, QObject *parent = nullptr);

    Q_INVOKABLE void fetchEntries();
    Q_INVOKABLE void fetchDetail(const AcademicCalendarEntry &entry);
    void invalidatePending();

    static QNetworkRequest buildRequest(const QUrl &url);
    static QString decodeHtml(const QByteArray &bytes, const QByteArray &contentType = {});
    static bool calendarPageExplicitlyEmpty(const QString &html);
    static QList<AcademicCalendarEntry> parseEntries(const QString &html);
    static QStringList parseImageUrls(const QString &html, const QString &baseUrl = QStringLiteral("https://jwc.scu.edu.cn"));

signals:
    void entriesFetched(const QList<AcademicCalendarEntry> &entries);
    void detailFetched(const AcademicCalendarDetail &detail);
    void failed(const ApiError &error);

private:
    QNetworkAccessManager *m_network = nullptr;
    quint64 m_listGeneration = 0;
    quint64 m_detailGeneration = 0;
};
