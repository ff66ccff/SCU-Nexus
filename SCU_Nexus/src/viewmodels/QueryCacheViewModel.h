#pragma once

#include "repositories/QueryCacheRepository.h"

#include <QObject>

class QueryCacheViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    explicit QueryCacheViewModel(QueryCacheRepository *cache, QObject *parent = nullptr);

    QString errorMessage() const;
    Q_INVOKABLE bool clearAll();

signals:
    void errorMessageChanged();
    void cacheCleared();

private:
    void setErrorMessage(const QString &message);

    QueryCacheRepository *m_cache = nullptr;
    QString m_errorMessage;
};
