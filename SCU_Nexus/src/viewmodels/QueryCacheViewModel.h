#pragma once

#include "repositories/QueryCacheRepository.h"

#include <QObject>

class QueryCacheViewModel : public QObject
{
    Q_OBJECT

public:
    explicit QueryCacheViewModel(QueryCacheRepository *cache, QObject *parent = nullptr);

    Q_INVOKABLE bool clearAll();

private:
    QueryCacheRepository *m_cache = nullptr;
};
