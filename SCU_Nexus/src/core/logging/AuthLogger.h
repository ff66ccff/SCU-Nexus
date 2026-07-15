#ifndef AUTHLOGGER_H
#define AUTHLOGGER_H

#include <QDateTime>
#include <QList>
#include <QString>

enum class AuthLogLevel {
    Debug,
    Info,
    Warn,
    Error
};

struct AuthLogEntry
{
    QDateTime timestamp;
    AuthLogLevel level = AuthLogLevel::Info;
    QString tag;
    QString message;

    QString format(bool includeDate = false) const;
};

// 日志写入前的最后一道脱敏防线；调用方仍应避免主动传入完整响应或凭据。
class AuthLogRedactor
{
public:
    static QString apply(QString text);
};

// 认证与教务网络链路的内存环形日志。
// 日志不直接落盘，只有用户显式导出时才生成文本；容量上限防止长期运行无限增长。
class AuthLogger
{
public:
    explicit AuthLogger(int capacity = 1000);

    static AuthLogger& instance();

    QList<AuthLogEntry> entries() const;
    void clear();
    void log(AuthLogLevel level, const QString& tag, const QString& message);
    void debug(const QString& tag, const QString& message);
    void info(const QString& tag, const QString& message);
    void warn(const QString& tag, const QString& message);
    void error(const QString& tag, const QString& message);
    QString exportToText(bool includeDate = true) const;
    static QString levelName(AuthLogLevel level);

private:
    int m_capacity = 1000;
    QList<AuthLogEntry> m_entries;
};

#endif // AUTHLOGGER_H
