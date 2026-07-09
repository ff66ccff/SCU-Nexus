#pragma once

#include "common/QueryState.h"
#include "models/AcademicCalendarModels.h"
#include "repositories/QueryCacheRepository.h"
#include "services/calendar/AcademicCalendarService.h"

#include <QDateTime>
#include <QObject>
#include <QVariantList>

class AcademicCalendarViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QueryStateNamespace::QueryState state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)
    Q_PROPERTY(QDateTime lastUpdated READ lastUpdated NOTIFY lastUpdatedChanged)
    Q_PROPERTY(bool hasCache READ hasCache NOTIFY cacheChanged)
    Q_PROPERTY(QVariantList entries READ entries NOTIFY entriesChanged)
    Q_PROPERTY(int selectedIndex READ selectedIndex NOTIFY selectedChanged)
    Q_PROPERTY(QString selectedTitle READ selectedTitle NOTIFY selectedChanged)
    Q_PROPERTY(QStringList imageUrls READ imageUrls NOTIFY imageUrlsChanged)

public:
    explicit AcademicCalendarViewModel(QueryCacheRepository *cache, QObject *parent = nullptr);

    QueryState state() const;
    bool loading() const;
    QString errorMessage() const;
    QDateTime lastUpdated() const;
    bool hasCache() const;
    QVariantList entries() const;
    int selectedIndex() const;
    QString selectedTitle() const;
    QStringList imageUrls() const;

    Q_INVOKABLE void load();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void clearCache();
    Q_INVOKABLE void selectEntry(int index);
    Q_INVOKABLE void reloadList();
    Q_INVOKABLE void reloadSelected();

signals:
    void stateChanged();
    void errorChanged();
    void lastUpdatedChanged();
    void cacheChanged();
    void entriesChanged();
    void selectedChanged();
    void imageUrlsChanged();
    void toastRequested(const QString &message);

private:
    void setState(QueryState state);
    void setError(const QString &message);
    void readCache();
    void writeEntriesCache();
    void writeSelectedCache();
    void writeImagesCache();
    void applyEntries(const QList<AcademicCalendarEntry> &entries, bool fromNetwork);
    void applyDetail(const AcademicCalendarDetail &detail, bool fromNetwork);

    QueryCacheRepository *m_cache = nullptr;
    AcademicCalendarService m_service;
    QueryState m_state = QueryState::Idle;
    QString m_errorMessage;
    QDateTime m_lastUpdated;
    bool m_hasCache = false;
    QList<AcademicCalendarEntry> m_entries;
    int m_selectedIndex = -1;
    QStringList m_imageUrls;
};
