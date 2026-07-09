#ifndef COOKIEJAR_H
#define COOKIEJAR_H

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QString>
#include <QUrl>

class CookieJar
{
public:
    void storeFromSetCookie(const QUrl& url, const QList<QByteArray>& setCookieHeaders);
    QString cookieHeader(const QUrl& url) const;
    QString cookieHeaderForDebug() const;
    void clear();

private:
    static QString normalizedHost(QString host);
    static QStringList splitCombinedSetCookieHeader(const QString& header);
    static bool isCookiePairAhead(const QString& text, qsizetype commaIndex);

    QMap<QString, QMap<QString, QString>> m_cookiesByHost;
};

#endif // COOKIEJAR_H
