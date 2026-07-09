#include "repositories/QueryCacheRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

// 构造对象并初始化依赖关系。
QueryCacheRepository::QueryCacheRepository(const QString &databasePath, QObject *parent)
    : QObject(parent),
      m_connectionName(QStringLiteral("query-cache-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces))),
      m_databasePath(databasePath)
{
}

// 释放数据库连接并从连接池中移除当前连接。
QueryCacheRepository::~QueryCacheRepository()
{
    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName);
        db.close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

// 打开数据库连接并初始化缓存表结构。
bool QueryCacheRepository::open()
{
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(m_databasePath);
    if (!db.open()) {
        m_lastError = db.lastError().text();
        return false;
    }

    QSqlQuery query(db);
    const bool ok = query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS query_cache ("
        "cache_key TEXT PRIMARY KEY,"
        "payload_json TEXT NOT NULL,"
        "updated_at TEXT NOT NULL"
        ")"));
    if (!ok) {
        m_lastError = query.lastError().text();
    }
    return ok;
}

// 写入或更新缓存记录。
bool QueryCacheRepository::put(const QString &key, const QString &payloadJson, const QDateTime &updatedAt)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "INSERT INTO query_cache(cache_key, payload_json, updated_at) VALUES(?, ?, ?) "
        "ON CONFLICT(cache_key) DO UPDATE SET payload_json = excluded.payload_json, updated_at = excluded.updated_at"));
    query.addBindValue(key);
    query.addBindValue(payloadJson);
    query.addBindValue(updatedAt.toUTC().toString(Qt::ISODateWithMs));
    const bool ok = query.exec();
    if (!ok) {
        m_lastError = query.lastError().text();
    }
    return ok;
}

// 读取指定资源并返回结果。
bool QueryCacheRepository::get(const QString &key, QueryCacheEntry *entry) const
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT cache_key, payload_json, updated_at FROM query_cache WHERE cache_key = ?"));
    query.addBindValue(key);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    if (!query.next()) {
        return false;
    }
    if (entry) {
        entry->key = query.value(0).toString();
        entry->payloadJson = query.value(1).toString();
        entry->updatedAt = QDateTime::fromString(query.value(2).toString(), Qt::ISODateWithMs);
    }
    return true;
}

// 移除指定数据记录。
bool QueryCacheRepository::remove(const QString &key)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("DELETE FROM query_cache WHERE cache_key = ?"));
    query.addBindValue(key);
    const bool ok = query.exec();
    if (!ok) {
        m_lastError = query.lastError().text();
    }
    return ok;
}

// 清理内部状态或持久化数据。
bool QueryCacheRepository::clear()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    const bool ok = query.exec(QStringLiteral("DELETE FROM query_cache"));
    if (!ok) {
        m_lastError = query.lastError().text();
    }
    return ok;
}

// 返回最近一次持久化操作的错误信息。
QString QueryCacheRepository::lastError() const
{
    return m_lastError;
}
