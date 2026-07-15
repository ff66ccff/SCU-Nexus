#include "CookieJar.h"

#include <QStringList>

// 使用响应 host 作为仓库键，而不是直接信任服务端声明的宽 Domain。
// 每段只读取第一个 name=value；Path、Expires、HttpOnly 等属性不属于请求头内容。
void CookieJar::storeFromSetCookie(const QUrl& url, const QList<QByteArray>& setCookieHeaders)
{
    const QString fallbackHost = normalizedHost(url.host());
    if (fallbackHost.isEmpty()) {
        return;
    }

    for (const QByteArray& rawHeader : setCookieHeaders) {
        const QString header = QString::fromUtf8(rawHeader);
        for (const QString& cookieText : splitCombinedSetCookieHeader(header)) {
            const QString trimmed = cookieText.trimmed();
            const qsizetype semicolon = trimmed.indexOf(';');
            const QString pair = (semicolon >= 0 ? trimmed.left(semicolon) : trimmed).trimmed();
            const qsizetype equals = pair.indexOf('=');
            if (equals <= 0) {
                continue;
            }

            const QString name = pair.left(equals).trimmed();
            const QString value = pair.mid(equals + 1).trimmed();
            if (!name.isEmpty()) {
                m_cookiesByHost[fallbackHost][name] = value;
            }
        }
    }
}

// 父域匹配是单向的：id.scu.edu.cn 的 Cookie 可发给 sub.id.scu.edu.cn，
// 但绝不会发给 zhjw.scu.edu.cn 这类兄弟域。
QString CookieJar::cookieHeader(const QUrl& url) const
{
    const QString host = normalizedHost(url.host());
    if (host.isEmpty()) {
        return {};
    }

    QMap<QString, QString> matchedCookies;
    for (auto it = m_cookiesByHost.cbegin(); it != m_cookiesByHost.cend(); ++it) {
        const QString jarHost = it.key();
        if (host == jarHost || host.endsWith("." + jarHost)) {
            for (auto cookie = it.value().cbegin(); cookie != it.value().cend(); ++cookie) {
                matchedCookies[cookie.key()] = cookie.value();
            }
        }
    }

    QStringList pairs;
    for (auto it = matchedCookies.cbegin(); it != matchedCookies.cend(); ++it) {
        pairs.append(it.key() + "=" + it.value());
    }
    return pairs.join("; ");
}

// 调试摘要刻意排除 host 和 value，避免日志成为会话凭据的旁路存储。
QString CookieJar::cookieSummaryForDebug() const
{
    qsizetype storedPairCount = 0;
    QStringList names;
    for (auto host = m_cookiesByHost.cbegin(); host != m_cookiesByHost.cend(); ++host) {
        for (auto cookie = host.value().cbegin(); cookie != host.value().cend(); ++cookie) {
            ++storedPairCount;
            if (!names.contains(cookie.key())) {
                names.append(cookie.key());
            }
        }
    }
    names.sort(Qt::CaseSensitive);
    return QStringLiteral("count=%1; names=%2").arg(storedPairCount).arg(names.join(QLatin1Char(',')));
}

void CookieJar::clear()
{
    m_cookiesByHost.clear();
}

// 规范化 Cookie 域名，移除协议无关的前导点。
QString CookieJar::normalizedHost(QString host)
{
    host = host.trimmed().toLower();
    while (host.startsWith('.')) {
        host.remove(0, 1);
    }
    return host;
}

// 不能用 split(',')：Expires=Wed, 21 Oct ... 自身包含逗号。这里只在逗号后
// 看起来确实出现新的 cookie-name= 时切分。
QStringList CookieJar::splitCombinedSetCookieHeader(const QString& header)
{
    QStringList result;
    qsizetype start = 0;
    for (qsizetype i = 0; i < header.size(); ++i) {
        if (header.at(i) == ',' && isCookiePairAhead(header, i)) {
            result.append(header.mid(start, i - start));
            start = i + 1;
        }
    }
    result.append(header.mid(start));
    return result;
}

bool CookieJar::isCookiePairAhead(const QString& text, qsizetype commaIndex)
{
    qsizetype i = commaIndex + 1;
    while (i < text.size() && text.at(i).isSpace()) {
        ++i;
    }

    if (i >= text.size() || !text.at(i).isLetter()) {
        return false;
    }

    for (; i < text.size(); ++i) {
        const QChar ch = text.at(i);
        if (ch == '=') {
            return true;
        }
        if (ch == ';' || ch == ',') {
            return false;
        }
        if (!(ch.isLetterOrNumber() || ch == '_' || ch == '-' || ch == '.')) {
            return false;
        }
    }
    return false;
}
