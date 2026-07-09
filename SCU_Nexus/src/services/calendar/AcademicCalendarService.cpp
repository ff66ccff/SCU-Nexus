#include "services/calendar/AcademicCalendarService.h"

#include <QNetworkReply>
#include <QRegularExpression>
#include <QStringConverter>

// 构造对象并初始化依赖关系。
AcademicCalendarService::AcademicCalendarService(QObject *parent)
    : QObject(parent)
{
}

// 发起数据获取流程并通过回调返回结果。
void AcademicCalendarService::fetchEntries()
{
    QNetworkReply *reply = m_network.get(QNetworkRequest(QUrl(QStringLiteral("https://jwc.scu.edu.cn/cdxl.htm"))));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit failed(reply->errorString());
            return;
        }
        emit entriesFetched(parseEntries(decodeHtml(reply->readAll(), reply->header(QNetworkRequest::ContentTypeHeader).toByteArray())));
    });
}

// 发起数据获取流程并通过回调返回结果。
void AcademicCalendarService::fetchDetail(const AcademicCalendarEntry &entry)
{
    const QUrl url(QStringLiteral("https://jwc.scu.edu.cn/") + entry.path);
    QNetworkReply *reply = m_network.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, entry]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit failed(reply->errorString());
            return;
        }
        AcademicCalendarDetail detail;
        detail.entry = entry;
        detail.imageUrls = parseImageUrls(decodeHtml(reply->readAll(), reply->header(QNetworkRequest::ContentTypeHeader).toByteArray()));
        emit detailFetched(detail);
    });
}

// 按响应编码解码校历 HTML 内容。
QString AcademicCalendarService::decodeHtml(const QByteArray &bytes, const QByteArray &contentType)
{
    const QByteArray lower = contentType.toLower();
    const int charsetIndex = lower.indexOf("charset=");
    if (charsetIndex >= 0) {
        const QByteArray charset = lower.mid(charsetIndex + 8).trimmed();
        const auto encoding = QStringConverter::encodingForName(charset);
        if (encoding.has_value()) {
            QStringDecoder decoder(*encoding);
            return decoder(bytes);
        }
    }

    const QString utf8 = QString::fromUtf8(bytes);
    if (!utf8.contains(QChar::ReplacementCharacter)) {
        return utf8;
    }

    const auto gb18030 = QStringConverter::encodingForName("GB18030");
    if (gb18030.has_value()) {
        QStringDecoder decoder(*gb18030);
        return decoder(bytes);
    }
    return QString::fromLocal8Bit(bytes);
}

// 解析外部数据并转换为内部结构。
QList<AcademicCalendarEntry> AcademicCalendarService::parseEntries(const QString &html)
{
    QList<AcademicCalendarEntry> entries;
    static const QRegularExpression re(
        QStringLiteral("<a[^>]+href=\"(info/1101/\\d+\\.htm)\"[^>]*>[^<]*?(\\d{4})-(\\d{4})[^<]*</a>"),
        QRegularExpression::CaseInsensitiveOption);

    QRegularExpressionMatchIterator it = re.globalMatch(html);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        entries.append({match.captured(2) + QStringLiteral("-") + match.captured(3), match.captured(1)});
    }
    return entries;
}

// 解析外部数据并转换为内部结构。
QStringList AcademicCalendarService::parseImageUrls(const QString &html, const QString &baseUrl)
{
    QStringList urls;
    static const QRegularExpression re(
        QStringLiteral("<img[^>]+src=\"(/__local/[^\"]+\\.(?:jpg|jpeg|png|gif|webp))\"[^>]*>"),
        QRegularExpression::CaseInsensitiveOption);

    QRegularExpressionMatchIterator it = re.globalMatch(html);
    while (it.hasNext()) {
        const QString path = it.next().captured(1);
        urls.append(baseUrl + path);
    }
    urls.removeDuplicates();
    return urls;
}
