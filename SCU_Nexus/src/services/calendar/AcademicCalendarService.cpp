#include "services/calendar/AcademicCalendarService.h"

#include "core/logging/AuthLogger.h"
#include "core/network/NetworkSettings.h"

#include <QNetworkReply>
#include <QRegularExpression>
#include <QStringConverter>
#include <iconv.h>
#include <optional>

namespace {

constexpr auto CalendarListUrl = "https://jwc.scu.edu.cn/cdxl.htm";
constexpr auto CalendarBaseUrl = "https://jwc.scu.edu.cn/";

ApiError makeError(ApiErrorType type,
                   const QString &message,
                   int statusCode = 0,
                   const QString &debugBody = {})
{
    return ApiError{type, message, statusCode, debugBody};
}

QString safeBodySummary(const QString &body)
{
    QString summary = AuthLogRedactor::apply(body.simplified());
    summary.replace(
        QRegularExpression(
            QStringLiteral(R"((\b(?:access_token|token|password|student|studentId|username)\s*[:=]\s*)[^&;\s,}\"'<>]+)"),
            QRegularExpression::CaseInsensitiveOption),
        QStringLiteral(R"(\1<redacted>)"));
    summary.replace(QRegularExpression(QStringLiteral(R"(\b\d{8,14}\b)")),
                    QStringLiteral("<redacted>"));
    return summary.left(500);
}

ApiError classifyReplyFailure(const QNetworkReply *reply)
{
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QNetworkReply::NetworkError networkError = reply->error();

    if (networkError == QNetworkReply::TimeoutError
        || networkError == QNetworkReply::OperationCanceledError) {
        return makeError(ApiErrorType::Timeout,
                         QStringLiteral("校历请求超时，请稍后重试。"),
                         statusCode);
    }
    if (statusCode == 401 || statusCode == 403) {
        return makeError(ApiErrorType::Unauthenticated,
                         QStringLiteral("校历服务拒绝访问，请重新登录后重试。"),
                         statusCode);
    }
    if (statusCode == 429) {
        return makeError(ApiErrorType::RateLimited,
                         QStringLiteral("校历请求过于频繁，请稍后重试。"),
                         statusCode);
    }
    if (statusCode >= 500 && statusCode <= 599) {
        return makeError(ApiErrorType::ServiceUnavailable,
                         QStringLiteral("校历服务暂时不可用，请稍后重试。"),
                         statusCode);
    }
    if (networkError != QNetworkReply::NoError) {
        return makeError(ApiErrorType::Network,
                         QStringLiteral("校历网络请求失败，请检查网络后重试。"),
                         statusCode);
    }
    return {};
}

std::optional<QString> decodeWithIconv(const QByteArray &bytes, const QByteArray &encodingName)
{
    iconv_t converter = iconv_open("UTF-8", encodingName.constData());
    if (converter == reinterpret_cast<iconv_t>(-1)) {
        return std::nullopt;
    }

    QByteArray output(qMax(32, bytes.size() * 4 + 4), '\0');
    char *inputData = const_cast<char *>(bytes.constData());
    size_t inputLeft = static_cast<size_t>(bytes.size());
    char *outputData = output.data();
    size_t outputLeft = static_cast<size_t>(output.size());
    const size_t result = iconv(converter, &inputData, &inputLeft, &outputData, &outputLeft);
    iconv_close(converter);
    if (result == static_cast<size_t>(-1) || inputLeft != 0) {
        return std::nullopt;
    }

    output.resize(static_cast<qsizetype>(outputData - output.data()));
    return QString::fromUtf8(output);
}

std::optional<QString> decodeWithNamedEncoding(const QByteArray &bytes,
                                               const QByteArray &encodingName)
{
    const auto qtEncoding = QStringConverter::encodingForName(encodingName);
    if (qtEncoding.has_value()) {
        QStringDecoder decoder(*qtEncoding);
        const QString decoded = decoder(bytes);
        if (!decoder.hasError()) {
            return decoded;
        }
    }
    return decodeWithIconv(bytes, encodingName);
}

ApiError parseFailure(const QString &message, int statusCode, const QString &html)
{
    const QString summary = safeBodySummary(html);
    AuthLogger::instance().warn(
        QStringLiteral("AcademicCalendar"),
        QStringLiteral("parse failed status=%1 summary=%2").arg(statusCode).arg(summary));
    return makeError(ApiErrorType::ParseFailed, message, statusCode, summary);
}

} // namespace

// 构造对象并初始化依赖关系。
AcademicCalendarService::AcademicCalendarService(QObject *parent)
    : QObject(parent),
      m_network(new QNetworkAccessManager(this))
{}

// 构造对象并使用调用方提供的网络边界。
AcademicCalendarService::AcademicCalendarService(QNetworkAccessManager *network, QObject *parent)
    : QObject(parent),
      m_network(network)
{
    Q_ASSERT(m_network);
}

// 使用统一网络策略构建校历请求。
QNetworkRequest AcademicCalendarService::buildRequest(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", NetworkSettings::kDefaultUserAgent.toUtf8());
    request.setRawHeader("Accept", "text/html,application/xhtml+xml");
    request.setTransferTimeout(NetworkSettings::kDefaultTimeoutMs);
    return request;
}

// 使当前尚未完成的校历请求失效。
void AcademicCalendarService::invalidatePending()
{
    ++m_listGeneration;
    ++m_detailGeneration;
}

// 发起数据获取流程并通过回调返回结果。
void AcademicCalendarService::fetchEntries()
{
    const quint64 generation = ++m_listGeneration;
    ++m_detailGeneration;
    QNetworkReply *reply = m_network->get(buildRequest(QUrl(QString::fromLatin1(CalendarListUrl))));
    connect(reply, &QNetworkReply::finished, this, [this, reply, generation]() {
        reply->deleteLater();
        if (generation != m_listGeneration) {
            return;
        }
        const ApiError replyError = classifyReplyFailure(reply);
        if (replyError.type != ApiErrorType::Unknown) {
            emit failed(replyError);
            return;
        }

        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QString html = decodeHtml(
            reply->readAll(), reply->header(QNetworkRequest::ContentTypeHeader).toByteArray());
        const QList<AcademicCalendarEntry> entries = parseEntries(html);
        if (entries.isEmpty() && !calendarPageExplicitlyEmpty(html)) {
            emit failed(parseFailure(QStringLiteral("校历列表页面结构无法识别"), statusCode, html));
            return;
        }
        emit entriesFetched(entries);
    });
}

// 发起数据获取流程并通过回调返回结果。
void AcademicCalendarService::fetchDetail(const AcademicCalendarEntry &entry)
{
    const quint64 generation = ++m_detailGeneration;
    const QUrl url(QString::fromLatin1(CalendarBaseUrl) + entry.path);
    QNetworkReply *reply = m_network->get(buildRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, entry, generation]() {
        reply->deleteLater();
        if (generation != m_detailGeneration) {
            return;
        }
        const ApiError replyError = classifyReplyFailure(reply);
        if (replyError.type != ApiErrorType::Unknown) {
            emit failed(replyError);
            return;
        }

        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QString html = decodeHtml(
            reply->readAll(), reply->header(QNetworkRequest::ContentTypeHeader).toByteArray());
        AcademicCalendarDetail detail;
        detail.entry = entry;
        detail.imageUrls = parseImageUrls(html);
        if (detail.imageUrls.isEmpty() && !calendarPageExplicitlyEmpty(html)) {
            emit failed(parseFailure(QStringLiteral("校历详情页面结构无法识别"), statusCode, html));
            return;
        }
        emit detailFetched(detail);
    });
}

// 按响应编码解码校历 HTML 内容。
QString AcademicCalendarService::decodeHtml(const QByteArray &bytes, const QByteArray &contentType)
{
    static const QRegularExpression charsetExpression(
        QStringLiteral(R"REGEX(charset\s*=\s*["']?([^;"'\s]+))REGEX"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = charsetExpression.match(QString::fromLatin1(contentType));
    if (match.hasMatch()) {
        const QByteArray encodingName = match.captured(1).toLatin1();
        if (const auto decoded = decodeWithNamedEncoding(bytes, encodingName); decoded.has_value()) {
            return *decoded;
        }
    }

    QStringDecoder utf8Decoder(QStringConverter::Utf8);
    const QString utf8 = utf8Decoder(bytes);
    if (!utf8Decoder.hasError()) {
        return utf8;
    }

    if (const auto gb18030 = decodeWithIconv(bytes, QByteArrayLiteral("GB18030"));
        gb18030.has_value()) {
        return *gb18030;
    }
    return QString::fromLocal8Bit(bytes);
}

// 识别服务端明确返回的校历空结果。
bool AcademicCalendarService::calendarPageExplicitlyEmpty(const QString &html)
{
    static const QStringList markers{
        QStringLiteral("暂无校历"),
        QStringLiteral("暂无数据"),
        QStringLiteral("没有查询到")
    };
    for (const QString &marker : markers) {
        if (html.contains(marker, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
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
        const QRegularExpressionMatch entryMatch = it.next();
        entries.append({entryMatch.captured(2) + QStringLiteral("-") + entryMatch.captured(3),
                        entryMatch.captured(1)});
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
