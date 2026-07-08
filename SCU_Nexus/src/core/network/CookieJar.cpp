#include "CookieJar.h"

#include <QStringList>

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

            // Domain isolation is required for SCU/ZHJW; path matching is a documented simplification for now.
            const QString attributes = semicolon >= 0 ? trimmed.mid(semicolon + 1) : QString();
            const QString host = cookieDomain(url, attributes);
            const QString name = pair.left(equals).trimmed();
            const QString value = pair.mid(equals + 1).trimmed();
            if (!host.isEmpty() && !name.isEmpty()) {
                m_cookiesByHost[host][name] = value;
            }
        }
    }
}

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

void CookieJar::clear()
{
    m_cookiesByHost.clear();
}

QString CookieJar::normalizedHost(QString host)
{
    host = host.trimmed().toLower();
    while (host.startsWith('.')) {
        host.remove(0, 1);
    }
    return host;
}

QString CookieJar::cookieDomain(const QUrl& url, const QString& attributes)
{
    const QString fallbackHost = normalizedHost(url.host());
    const QStringList parts = attributes.split(';', Qt::SkipEmptyParts);
    for (const QString& part : parts) {
        const QString attr = part.trimmed();
        const qsizetype equals = attr.indexOf('=');
        if (equals <= 0) {
            continue;
        }
        if (attr.left(equals).trimmed().compare("Domain", Qt::CaseInsensitive) == 0) {
            const QString domain = normalizedHost(attr.mid(equals + 1));
            return domain.isEmpty() ? fallbackHost : domain;
        }
    }
    return fallbackHost;
}

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
