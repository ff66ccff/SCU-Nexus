#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <QByteArray>
#include <QMap>
#include <QString>
#include <QUrl>

// 网络层统一响应。即使状态码为 4xx/5xx，也会保留 body 供认证/API 层解析
// 服务端的业务错误；finalUrl 是手动重定向链最终实际访问的地址。
struct HttpResponse
{
    int statusCode = 0;
    QByteArray body;
    QMap<QString, QString> headers;
    QUrl finalUrl;
};

#endif // HTTPRESPONSE_H
