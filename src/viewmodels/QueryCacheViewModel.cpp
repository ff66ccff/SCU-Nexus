#include "viewmodels/QueryCacheViewModel.h"

QueryCacheViewModel::QueryCacheViewModel(QueryCacheRepository *cache, QObject *parent)
    : QObject(parent),
      m_cache(cache)
{
}

bool QueryCacheViewModel::clearAll()
{
    return m_cache && m_cache->clear();
}
