#include "AuthLogger.h"

#include <QRegularExpression>
#include <QStringList>

namespace {

QString twoDigits(int value)
{
    return QString::number(value).rightJustified(2, QLatin1Char('0'));
}

QString threeDigits(int value)
{
    return QString::number(value).rightJustified(3, QLatin1Char('0'));
}

} // namespace

QString AuthLogEntry::format(bool includeDate) const
{
    const QString time = QStringLiteral("%1:%2:%3.%4")
        .arg(twoDigits(timestamp.time().hour()),
             twoDigits(timestamp.time().minute()),
             twoDigits(timestamp.time().second()),
             threeDigits(timestamp.time().msec()));
    const QString stamp = includeDate
        ? QStringLiteral("%1 %2").arg(timestamp.date().toString(QStringLiteral("yyyy-MM-dd")), time)
        : time;
    return QStringLiteral("%1 %2 [%3] %4")
        .arg(stamp,
             AuthLogger::levelName(level).leftJustified(5, QLatin1Char(' ')),
             tag,
             message);
}

QString AuthLogRedactor::apply(QString text)
{
    text.replace(QRegularExpression(QStringLiteral(R"REGEX(("access_token"\s*:\s*)"[^"]*")REGEX"),
                                    QRegularExpression::CaseInsensitiveOption),
                 QStringLiteral(R"(\1"<redacted>")"));
    text.replace(QRegularExpression(QStringLiteral(R"REGEX(("password"\s*:\s*)"[^"]*")REGEX"),
                                    QRegularExpression::CaseInsensitiveOption),
                 QStringLiteral(R"(\1"<redacted>")"));
    text.replace(QRegularExpression(QStringLiteral(R"((Bearer\s+)[A-Za-z0-9._\-]+)"),
                                    QRegularExpression::CaseInsensitiveOption),
                 QStringLiteral(R"(\1<redacted>)"));

    QRegularExpression codeExpression(QStringLiteral(R"(([?&](?:code|access_token)=)([^&\s"]+))"),
                                      QRegularExpression::CaseInsensitiveOption);
    qsizetype offset = 0;
    QString result;
    while (true) {
        const QRegularExpressionMatch match = codeExpression.match(text, offset);
        if (!match.hasMatch()) {
            result += text.mid(offset);
            break;
        }
        result += text.mid(offset, match.capturedStart() - offset);
        const QString prefix = match.captured(1);
        const QString value = match.captured(2);
        result += prefix + (value.size() <= 4 ? QStringLiteral("<redacted>") : value.left(4) + QStringLiteral("..."));
        offset = match.capturedEnd();
    }
    return result;
}

AuthLogger::AuthLogger(int capacity)
    : m_capacity(qMax(1, capacity))
{
}

AuthLogger& AuthLogger::instance()
{
    static AuthLogger logger;
    return logger;
}

QList<AuthLogEntry> AuthLogger::entries() const
{
    return m_entries;
}

void AuthLogger::clear()
{
    m_entries.clear();
}

void AuthLogger::log(AuthLogLevel level, const QString& tag, const QString& message)
{
    AuthLogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.level = level;
    entry.tag = tag;
    entry.message = AuthLogRedactor::apply(message);
    m_entries.append(entry);
    while (m_entries.size() > m_capacity) {
        m_entries.removeFirst();
    }
}

void AuthLogger::debug(const QString& tag, const QString& message)
{
    log(AuthLogLevel::Debug, tag, message);
}

void AuthLogger::info(const QString& tag, const QString& message)
{
    log(AuthLogLevel::Info, tag, message);
}

void AuthLogger::warn(const QString& tag, const QString& message)
{
    log(AuthLogLevel::Warn, tag, message);
}

void AuthLogger::error(const QString& tag, const QString& message)
{
    log(AuthLogLevel::Error, tag, message);
}

QString AuthLogger::exportToText(bool includeDate) const
{
    QStringList lines;
    if (includeDate) {
        lines.append(QStringLiteral("# SCU Nexus auth log"));
        lines.append(QStringLiteral("# exported: %1").arg(QDateTime::currentDateTime().toString(Qt::ISODate)));
        lines.append(QStringLiteral("# entries: %1").arg(m_entries.size()));
        lines.append(QString());
    }
    for (const AuthLogEntry& entry : m_entries) {
        lines.append(entry.format(includeDate));
    }
    return lines.join(QLatin1Char('\n'));
}

QString AuthLogger::levelName(AuthLogLevel level)
{
    switch (level) {
    case AuthLogLevel::Debug: return QStringLiteral("DEBUG");
    case AuthLogLevel::Info: return QStringLiteral("INFO");
    case AuthLogLevel::Warn: return QStringLiteral("WARN");
    case AuthLogLevel::Error: return QStringLiteral("ERROR");
    }
    return QStringLiteral("INFO");
}
