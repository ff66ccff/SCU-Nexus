#include "CookieJar.h"

#include <QStringList>

// 解析响应中的 Set-Cookie 头并写入 Cookie 仓库。
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

// 处理 Cookie 的解析、存储或输出。
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

// 处理 Cookie 的解析、存储或输出。
QString CookieJar::cookieHeaderForDebug() const
{
    QStringList pairs;
    for (auto host = m_cookiesByHost.cbegin(); host != m_cookiesByHost.cend(); ++host) {
        for (auto cookie = host.value().cbegin(); cookie != host.value().cend(); ++cookie) {
            pairs.append(cookie.key() + "=" + cookie.value());
        }
    }
    return pairs.join("; ");
}

// 清理内部状态或持久化数据。
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

// 拆分可能被合并在同一行里的 Set-Cookie 字段。
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

// 判断条件是否成立并返回布尔结果。
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
