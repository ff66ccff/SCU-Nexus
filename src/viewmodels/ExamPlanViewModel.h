#pragma once

#include "common/QueryState.h"
#include "models/ExamPlanModels.h"
#include "repositories/QueryCacheRepository.h"
#include "services/zhjw/MockZhjwApiService.h"

#include <QDateTime>
#include <QObject>
#include <QVariantList>

class ExamPlanViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QueryStateNamespace::QueryState state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)
    Q_PROPERTY(QDateTime lastUpdated READ lastUpdated NOTIFY lastUpdatedChanged)
    Q_PROPERTY(bool hasCache READ hasCache NOTIFY cacheChanged)
    Q_PROPERTY(int count READ count NOTIFY examsChanged)
    Q_PROPERTY(QVariantList exams READ exams NOTIFY examsChanged)
    Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY authChanged)

public:
    explicit ExamPlanViewModel(QueryCacheRepository *cache, MockZhjwApiService *api, QObject *parent = nullptr);

    QueryState state() const;
    bool loading() const;
    QString errorMessage() const;
    QDateTime lastUpdated() const;
    bool hasCache() const;
    int count() const;
    QVariantList exams() const;
    bool loggedIn() const;

    Q_INVOKABLE void load();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void clearCache();
    Q_INVOKABLE QVariantMap examAt(int index) const;

signals:
    void stateChanged();
    void errorChanged();
    void lastUpdatedChanged();
    void cacheChanged();
    void examsChanged();
    void authChanged();
    void toastRequested(const QString &message);

private:
    void setState(QueryState state);
    void setError(const QString &message);
    void readCache();
    void writeCache();

    QueryCacheRepository *m_cache = nullptr;
    MockZhjwApiService *m_api = nullptr;
    QueryState m_state = QueryState::Idle;
    QString m_errorMessage;
    QDateTime m_lastUpdated;
    bool m_hasCache = false;
    QList<ExamPlanItem> m_exams;
};
