#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <QByteArray>
#include <QMap>
#include <QString>
#include <QUrl>

// 与具体 Qt Reply 解耦的请求描述，供网络层扩展和测试桩使用。
struct HttpRequest
{
    QString method = "GET";
    QUrl url;
    QByteArray body;
    QMap<QString, QString> headers;
};

#endif // HTTPREQUEST_H
