#pragma once

#include <QDateTime>
#include <QObject>
#include <QSqlDatabase>
#include <QString>

struct QueryCacheEntry {
    QString key;
    QString payloadJson;
    QDateTime updatedAt;
};

class QueryCacheRepository : public QObject
{
    Q_OBJECT

public:
    explicit QueryCacheRepository(const QString &databasePath, QObject *parent = nullptr);
    ~QueryCacheRepository() override;

    bool open();
    bool put(const QString &key, const QString &payloadJson, const QDateTime &updatedAt = QDateTime::currentDateTimeUtc());
    bool get(const QString &key, QueryCacheEntry *entry) const;
    bool remove(const QString &key);
    bool clear();
    QString lastError() const;

private:
    QString m_connectionName;
    QString m_databasePath;
    mutable QString m_lastError;
};
