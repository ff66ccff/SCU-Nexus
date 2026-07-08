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

class AuthLogRedactor
{
public:
    static QString apply(QString text);
};

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
