#include "viewmodels/QueryCacheViewModel.h"

// 构造对象并初始化依赖关系。
QueryCacheViewModel::QueryCacheViewModel(QueryCacheRepository *cache, QObject *parent)
    : QObject(parent),
      m_cache(cache)
{
}

// 返回最近一次缓存清理错误。
QString QueryCacheViewModel::errorMessage() const
{
    return m_errorMessage;
}

// 清理内部状态或持久化数据。
bool QueryCacheViewModel::clearAll()
{
    setErrorMessage({});
    if (!m_cache) {
        setErrorMessage(QStringLiteral("缓存仓库未初始化"));
        return false;
    }
    if (!m_cache->clear()) {
        setErrorMessage(m_cache->lastError());
        return false;
    }

    emit cacheCleared();
    return true;
}

// 更新最近一次缓存清理错误并通知界面。
void QueryCacheViewModel::setErrorMessage(const QString &message)
{
    if (m_errorMessage == message) {
        return;
    }
    m_errorMessage = message;
    emit errorMessageChanged();
}
