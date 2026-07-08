#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <QByteArray>
#include <QMap>
#include <QString>
#include <QUrl>

struct HttpRequest
{
    QString method = "GET";
    QUrl url;
    QByteArray body;
    QMap<QString, QString> headers;
};

#endif // HTTPREQUEST_H
