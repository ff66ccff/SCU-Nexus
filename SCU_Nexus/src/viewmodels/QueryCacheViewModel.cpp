#include "viewmodels/QueryCacheViewModel.h"

// 构造对象并初始化依赖关系。
QueryCacheViewModel::QueryCacheViewModel(QueryCacheRepository *cache, QObject *parent)
    : QObject(parent),
      m_cache(cache)
{
}

// 清理内部状态或持久化数据。
bool QueryCacheViewModel::clearAll()
{
    return m_cache && m_cache->clear();
}
