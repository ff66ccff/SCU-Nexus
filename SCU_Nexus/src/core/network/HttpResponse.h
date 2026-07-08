#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <QByteArray>
#include <QMap>
#include <QString>
#include <QUrl>

struct HttpResponse
{
    int statusCode = 0;
    QByteArray body;
    QMap<QString, QString> headers;
    QUrl finalUrl;
};

#endif // HTTPRESPONSE_H
