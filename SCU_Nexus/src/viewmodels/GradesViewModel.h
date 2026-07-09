#pragma once

#include "common/QueryState.h"
#include "models/GradeModels.h"
#include "repositories/QueryCacheRepository.h"
#include "services/grades/GradeStatisticsService.h"
#include "services/zhjw/ZhjwQueryService.h"

#include <QDateTime>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class GradesViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QueryStateNamespace::QueryState schemeState READ schemeState NOTIFY schemeChanged)
    Q_PROPERTY(QueryStateNamespace::QueryState passingState READ passingState NOTIFY passingChanged)
    Q_PROPERTY(QString schemeErrorMessage READ schemeErrorMessage NOTIFY schemeChanged)
    Q_PROPERTY(QString passingErrorMessage READ passingErrorMessage NOTIFY passingChanged)
    Q_PROPERTY(QDateTime schemeLastUpdated READ schemeLastUpdated NOTIFY schemeChanged)
    Q_PROPERTY(QDateTime passingLastUpdated READ passingLastUpdated NOTIFY passingChanged)
    Q_PROPERTY(QVariantMap schemeSummary READ schemeSummary NOTIFY schemeChanged)
    Q_PROPERTY(QVariantMap passingSummary READ passingSummary NOTIFY passingChanged)
    Q_PROPERTY(QVariantList schemeGroups READ schemeGroups NOTIFY schemeChanged)
    Q_PROPERTY(QVariantList passingGroups READ passingGroups NOTIFY passingChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)
    Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY authChanged)

public:
    explicit GradesViewModel(QueryCacheRepository *cache, ZhjwQueryService *api, QObject *parent = nullptr);

    QueryState schemeState() const;
    QueryState passingState() const;
    QString schemeErrorMessage() const;
    QString passingErrorMessage() const;
    QDateTime schemeLastUpdated() const;
    QDateTime passingLastUpdated() const;
    QVariantMap schemeSummary() const;
    QVariantMap passingSummary() const;
    QVariantList schemeGroups() const;
    QVariantList passingGroups() const;
    QString searchQuery() const;
    bool loggedIn() const;

    Q_INVOKABLE void load();
    Q_INVOKABLE void refreshSchemeScores();
    Q_INVOKABLE void refreshPassingScores();
    Q_INVOKABLE void clearSchemeError();
    Q_INVOKABLE void clearPassingError();
    Q_INVOKABLE void setSearchQuery(QString query);
    Q_INVOKABLE QVariantList filteredSchemeGroups() const;
    Q_INVOKABLE QVariantList filteredPassingGroups() const;
    Q_INVOKABLE QVariantList filteredSchemeGroupsByAttr(const QString &attr) const;
    Q_INVOKABLE QVariantList filteredPassingGroupsByAttr(const QString &attr) const;
    Q_INVOKABLE QVariantMap customStatsForSelected(QVariantList selectedKeys) const;
    Q_INVOKABLE void clearCache();

signals:
    void schemeChanged();
    void passingChanged();
    void searchQueryChanged();
    void authChanged();
    void toastRequested(const QString &message);

private:
    void readSchemeCache();
    void readPassingCache();
    void writeSchemeCache(const QJsonObject &root);
    void writePassingCache(const QJsonObject &root);
    QList<GradeCourseItem> filteredItems(const QList<GradeCourseItem> &items) const;
    void handleSchemeError(const ApiError &error);
    void handlePassingError(const ApiError &error);

    QueryCacheRepository *m_cache = nullptr;
    ZhjwQueryService *m_api = nullptr;
    GradeStatisticsService m_stats;
    QueryState m_schemeState = QueryState::Idle;
    QueryState m_passingState = QueryState::Idle;
    QString m_schemeError;
    QString m_passingError;
    QDateTime m_schemeLastUpdated;
    QDateTime m_passingLastUpdated;
    QString m_searchQuery;
    SchemeScoreSummary m_scheme;
    PassingScoreResult m_passing;
    bool m_hasSchemeCache = false;
    bool m_hasPassingCache = false;
};
