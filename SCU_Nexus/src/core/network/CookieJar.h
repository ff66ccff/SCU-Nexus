#ifndef COOKIEJAR_H
#define COOKIEJAR_H

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QString>
#include <QUrl>

// 认证链路使用的轻量 Cookie 仓库。
//
// 第一阶段仅按“响应 host”隔离 Cookie，并允许其发送给该 host 的子域。
// 它有意不采纳 Set-Cookie 的 Domain/Path 属性：这样可以防止统一认证站点下发的
// 宽域 Cookie 意外泄漏到其他 scu.edu.cn 子系统。若以后需要完整浏览器语义，应优先
// 替换为经过审计的 Cookie 实现，而不是在这里零散追加规则。
class CookieJar
{
public:
    // 保存响应的全部 Set-Cookie 头；兼容代理把多个头合并成一行的情况。
    void storeFromSetCookie(const QUrl& url, const QList<QByteArray>& setCookieHeaders);
    // 仅返回与目标 host 精确匹配或父域匹配的 Cookie 请求头值。
    QString cookieHeader(const QUrl& url) const;
    // 只暴露数量和名称，禁止将 Cookie 值或 host 写入日志。
    QString cookieSummaryForDebug() const;
    void clear();

private:
    static QString normalizedHost(QString host);
    static QStringList splitCombinedSetCookieHeader(const QString& header);
    static bool isCookiePairAhead(const QString& text, qsizetype commaIndex);

    QMap<QString, QMap<QString, QString>> m_cookiesByHost;
};

#endif // COOKIEJAR_H
