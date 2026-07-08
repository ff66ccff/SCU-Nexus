#include "viewmodels/GradesViewModel.h"

#include <QJsonDocument>
#include <QSet>

namespace {
constexpr auto SchemeKey = "grades.scheme_scores";
constexpr auto PassingKey = "grades.passing_scores";
}

GradesViewModel::GradesViewModel(QueryCacheRepository *cache, MockZhjwApiService *api, QObject *parent)
    : QObject(parent),
      m_cache(cache),
      m_api(api)
{
    if (m_api) {
        connect(m_api, &MockZhjwApiService::loggedInChanged, this, &GradesViewModel::authChanged);
    }
}

QueryState GradesViewModel::schemeState() const { return m_schemeState; }
QueryState GradesViewModel::passingState() const { return m_passingState; }
QString GradesViewModel::schemeErrorMessage() const { return m_schemeError; }
QString GradesViewModel::passingErrorMessage() const { return m_passingError; }
QDateTime GradesViewModel::schemeLastUpdated() const { return m_schemeLastUpdated; }
QDateTime GradesViewModel::passingLastUpdated() const { return m_passingLastUpdated; }
QVariantMap GradesViewModel::schemeSummary() const { return m_scheme.toSummaryVariant(); }
QVariantMap GradesViewModel::passingSummary() const { return m_passing.toSummaryVariant(); }
QVariantList GradesViewModel::schemeGroups() const { return termGroupsToVariant(m_scheme.groupedByTerm()); }
QVariantList GradesViewModel::passingGroups() const { return passingGroupsToVariant(m_passing.groups); }
QString GradesViewModel::searchQuery() const { return m_searchQuery; }
bool GradesViewModel::loggedIn() const { return m_api && m_api->loggedIn(); }

void GradesViewModel::load()
{
    readSchemeCache();
    readPassingCache();
    if (!loggedIn() && !m_hasSchemeCache) {
        m_schemeState = QueryState::LoginRequired;
        emit schemeChanged();
    }
    if (!loggedIn() && !m_hasPassingCache) {
        m_passingState = QueryState::LoginRequired;
        emit passingChanged();
    }
    if (loggedIn()) {
        if (!m_hasSchemeCache) {
            refreshSchemeScores();
        }
        if (!m_hasPassingCache) {
            refreshPassingScores();
        }
    }
}

void GradesViewModel::refreshSchemeScores()
{
    if (!loggedIn()) {
        m_schemeState = m_hasSchemeCache ? QueryState::Loaded : QueryState::LoginRequired;
        emit schemeChanged();
        if (m_hasSchemeCache) {
            emit toastRequested(QStringLiteral("登录已失效，正在显示缓存方案成绩。"));
        }
        return;
    }

    m_schemeState = QueryState::Loading;
    emit schemeChanged();
    const QJsonObject root = m_api->fetchSchemeScores();
    m_scheme = m_stats.parseSchemeScores(root);
    m_hasSchemeCache = true;
    m_schemeLastUpdated = QDateTime::currentDateTimeUtc();
    writeSchemeCache(root);
    m_schemeState = m_scheme.items.isEmpty() ? QueryState::Empty : QueryState::Loaded;
    emit schemeChanged();
}

void GradesViewModel::refreshPassingScores()
{
    if (!loggedIn()) {
        m_passingState = m_hasPassingCache ? QueryState::Loaded : QueryState::LoginRequired;
        emit passingChanged();
        if (m_hasPassingCache) {
            emit toastRequested(QStringLiteral("登录已失效，正在显示缓存及格成绩。"));
        }
        return;
    }

    m_passingState = QueryState::Loading;
    emit passingChanged();
    const QJsonObject root = m_api->fetchPassingScores();
    m_passing = m_stats.parsePassingScores(root);
    m_hasPassingCache = true;
    m_passingLastUpdated = QDateTime::currentDateTimeUtc();
    writePassingCache(root);
    m_passingState = m_passing.groups.isEmpty() ? QueryState::Empty : QueryState::Loaded;
    emit passingChanged();
}

void GradesViewModel::clearSchemeError()
{
    m_schemeError.clear();
    emit schemeChanged();
}

void GradesViewModel::clearPassingError()
{
    m_passingError.clear();
    emit passingChanged();
}

void GradesViewModel::setSearchQuery(QString query)
{
    query = query.trimmed();
    if (m_searchQuery == query) {
        return;
    }
    m_searchQuery = query;
    emit searchQueryChanged();
    emit schemeChanged();
    emit passingChanged();
}

QVariantList GradesViewModel::filteredSchemeGroups() const
{
    return termGroupsToVariant(groupGradeCoursesByTerm(filteredItems(m_scheme.items)));
}

QVariantList GradesViewModel::filteredPassingGroups() const
{
    PassingScoreResult filtered;
    for (const PassingScoreGroup &group : m_passing.groups) {
        PassingScoreGroup next;
        next.label = group.label;
        next.items = filteredItems(group.items);
        if (!next.items.isEmpty()) {
            filtered.groups.append(next);
        }
    }
    return passingGroupsToVariant(filtered.groups);
}

QVariantMap GradesViewModel::customStatsForSelected(QVariantList selectedKeys) const
{
    QSet<QString> keys;
    for (const QVariant &value : selectedKeys) {
        keys.insert(value.toString());
    }

    QList<GradeCourseItem> selected;
    for (const GradeCourseItem &item : m_scheme.items) {
        if (keys.contains(item.key())) {
            selected.append(item);
        }
    }
    return m_stats.customStats(selected);
}

void GradesViewModel::clearCache()
{
    if (m_cache) {
        m_cache->remove(QString::fromLatin1(SchemeKey));
        m_cache->remove(QString::fromLatin1(PassingKey));
    }
    m_scheme = {};
    m_passing = {};
    m_hasSchemeCache = false;
    m_hasPassingCache = false;
    m_schemeState = QueryState::Idle;
    m_passingState = QueryState::Idle;
    emit schemeChanged();
    emit passingChanged();
}

void GradesViewModel::readSchemeCache()
{
    if (!m_cache) {
        return;
    }
    QueryCacheEntry entry;
    if (!m_cache->get(QString::fromLatin1(SchemeKey), &entry)) {
        return;
    }
    m_scheme = m_stats.parseSchemeScores(QJsonDocument::fromJson(entry.payloadJson.toUtf8()).object());
    m_schemeLastUpdated = entry.updatedAt;
    m_hasSchemeCache = true;
    m_schemeState = m_scheme.items.isEmpty() ? QueryState::Empty : QueryState::Loaded;
    emit schemeChanged();
}

void GradesViewModel::readPassingCache()
{
    if (!m_cache) {
        return;
    }
    QueryCacheEntry entry;
    if (!m_cache->get(QString::fromLatin1(PassingKey), &entry)) {
        return;
    }
    m_passing = m_stats.parsePassingScores(QJsonDocument::fromJson(entry.payloadJson.toUtf8()).object());
    m_passingLastUpdated = entry.updatedAt;
    m_hasPassingCache = true;
    m_passingState = m_passing.groups.isEmpty() ? QueryState::Empty : QueryState::Loaded;
    emit passingChanged();
}

void GradesViewModel::writeSchemeCache(const QJsonObject &root)
{
    if (m_cache) {
        m_cache->put(QString::fromLatin1(SchemeKey), QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact)));
    }
}

void GradesViewModel::writePassingCache(const QJsonObject &root)
{
    if (m_cache) {
        m_cache->put(QString::fromLatin1(PassingKey), QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact)));
    }
}

QList<GradeCourseItem> GradesViewModel::filteredItems(const QList<GradeCourseItem> &items) const
{
    if (m_searchQuery.isEmpty()) {
        return items;
    }
    QList<GradeCourseItem> filtered;
    for (const GradeCourseItem &item : items) {
        if (item.courseName.contains(m_searchQuery, Qt::CaseInsensitive) ||
            item.englishCourseName.contains(m_searchQuery, Qt::CaseInsensitive)) {
            filtered.append(item);
        }
    }
    return filtered;
}

static QList<GradeCourseItem> filterItemsByAttr(const QList<GradeCourseItem> &items, const QString &attr)
{
    if (attr.isEmpty() || attr == QStringLiteral("全部")) {
        return items;
    }
    QList<GradeCourseItem> filtered;
    for (const GradeCourseItem &item : items) {
        if (item.courseAttributeName == attr) {
            filtered.append(item);
        }
    }
    return filtered;
}

QVariantList GradesViewModel::filteredSchemeGroupsByAttr(const QString &attr) const
{
    const QList<GradeCourseItem> items = filterItemsByAttr(filteredItems(m_scheme.items), attr);
    return termGroupsToVariant(groupGradeCoursesByTerm(items));
}

QVariantList GradesViewModel::filteredPassingGroupsByAttr(const QString &attr) const
{
    PassingScoreResult filtered;
    for (const PassingScoreGroup &group : m_passing.groups) {
        PassingScoreGroup next;
        next.label = group.label;
        next.items = filterItemsByAttr(filteredItems(group.items), attr);
        if (!next.items.isEmpty()) {
            filtered.groups.append(next);
        }
    }
    return passingGroupsToVariant(filtered.groups);
}
