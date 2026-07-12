#include "viewmodels/GradesViewModel.h"

#include <QJsonDocument>
#include <QSet>

namespace {
constexpr auto SchemeKey = "grades.scheme_scores";
constexpr auto PassingKey = "grades.passing_scores";
const QString DamagedSchemeCacheMessage = QStringLiteral("方案成绩缓存已损坏，已移除。");
const QString DamagedPassingCacheMessage = QStringLiteral("及格成绩缓存已损坏，已移除。");
const QString DamagedSchemeRemovalMessage = QStringLiteral("移除损坏的方案成绩缓存失败，请重试。");
const QString DamagedPassingRemovalMessage = QStringLiteral("移除损坏的及格成绩缓存失败，请重试。");
const QString SchemeCacheReadMessage = QStringLiteral("读取方案成绩缓存失败，请重试。");
const QString PassingCacheReadMessage = QStringLiteral("读取及格成绩缓存失败，请重试。");
const QString SchemeCacheRemovalMessage = QStringLiteral("清除方案成绩缓存失败，请重试。");
const QString PassingCacheRemovalMessage = QStringLiteral("清除及格成绩缓存失败，请重试。");
const QString CacheRemovalFailureMessage = QStringLiteral("清除成绩缓存失败，请重试。");

bool parseGradeCachePayload(const QString &payload, QJsonObject *root)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return false;
    }
    const QJsonObject object = document.object();
    if (!object.value(QStringLiteral("lnList")).isArray()) {
        return false;
    }
    *root = object;
    return true;
}
}

// 构造对象并初始化依赖关系。
GradesViewModel::GradesViewModel(QueryCacheRepository *cache, ZhjwQueryService *api, QObject *parent)
    : QObject(parent),
      m_cache(cache),
      m_api(api)
{
    if (m_api) {
        connect(m_api, &ZhjwQueryService::loggedInChanged, this, &GradesViewModel::authChanged);
    }
}

// 返回方案成绩查询状态。
QueryState GradesViewModel::schemeState() const { return m_schemeState; }
// 返回及格成绩查询状态。
QueryState GradesViewModel::passingState() const { return m_passingState; }
// 返回方案成绩错误信息。
QString GradesViewModel::schemeErrorMessage() const { return m_schemeError; }
// 返回及格成绩错误信息。
QString GradesViewModel::passingErrorMessage() const { return m_passingError; }
// 返回方案成绩最近更新时间。
QDateTime GradesViewModel::schemeLastUpdated() const { return m_schemeLastUpdated; }
// 返回及格成绩最近更新时间。
QDateTime GradesViewModel::passingLastUpdated() const { return m_passingLastUpdated; }
// 返回方案成绩统计摘要。
QVariantMap GradesViewModel::schemeSummary() const { return m_scheme.toSummaryVariant(); }
// 返回及格成绩统计摘要。
QVariantMap GradesViewModel::passingSummary() const { return m_passing.toSummaryVariant(); }
// 返回按学期分组的方案成绩。
QVariantList GradesViewModel::schemeGroups() const { return termGroupsToVariant(m_scheme.groupedByTerm()); }
// 返回按学期分组的及格成绩。
QVariantList GradesViewModel::passingGroups() const { return passingGroupsToVariant(m_passing.groups); }
// 返回当前成绩搜索关键字。
QString GradesViewModel::searchQuery() const { return m_searchQuery; }
// 返回当前登录状态。
bool GradesViewModel::loggedIn() const { return m_api && m_api->loggedIn(); }

// 加载当前模块数据并同步界面状态。
void GradesViewModel::load()
{
    readSchemeCache();
    readPassingCache();
    if (!loggedIn() && !m_hasSchemeCache && m_schemeState != QueryState::LoginRequired) {
        m_schemeState = QueryState::LoginRequired;
        emit schemeChanged();
    }
    if (!loggedIn() && !m_hasPassingCache && m_passingState != QueryState::LoginRequired) {
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

// 刷新远端数据并更新缓存状态。
void GradesViewModel::refreshSchemeScores()
{
    if (m_schemeState == QueryState::Loading || m_schemeState == QueryState::Refreshing) {
        return;
    }
    if (!loggedIn()) {
        m_schemeState = m_hasSchemeCache
            ? (m_scheme.items.isEmpty() ? QueryState::Empty : QueryState::Loaded)
            : QueryState::LoginRequired;
        emit schemeChanged();
        if (m_hasSchemeCache) {
            emit toastRequested(QStringLiteral("登录已失效，正在显示缓存方案成绩。"));
        }
        return;
    }

    m_schemeError.clear();
    m_schemeState = m_hasSchemeCache ? QueryState::Refreshing : QueryState::Loading;
    emit schemeChanged();
    m_api->fetchSchemeScores([this](const QJsonObject &root, const ApiError &error) {
        if (error.type != ApiErrorType::Unknown) {
            handleSchemeError(error);
            return;
        }
        m_scheme = m_stats.parseSchemeScores(root);
        m_hasSchemeCache = true;
        m_schemeLastUpdated = QDateTime::currentDateTimeUtc();
        m_schemeError.clear();
        writeSchemeCache(root);
        m_schemeState = m_scheme.items.isEmpty() ? QueryState::Empty : QueryState::Loaded;
        emit schemeChanged();
    });
}

// 刷新远端数据并更新缓存状态。
void GradesViewModel::refreshPassingScores()
{
    if (m_passingState == QueryState::Loading || m_passingState == QueryState::Refreshing) {
        return;
    }
    if (!loggedIn()) {
        m_passingState = m_hasPassingCache
            ? (m_passing.groups.isEmpty() ? QueryState::Empty : QueryState::Loaded)
            : QueryState::LoginRequired;
        emit passingChanged();
        if (m_hasPassingCache) {
            emit toastRequested(QStringLiteral("登录已失效，正在显示缓存及格成绩。"));
        }
        return;
    }

    m_passingError.clear();
    m_passingState = m_hasPassingCache ? QueryState::Refreshing : QueryState::Loading;
    emit passingChanged();
    m_api->fetchPassingScores([this](const QJsonObject &root, const ApiError &error) {
        if (error.type != ApiErrorType::Unknown) {
            handlePassingError(error);
            return;
        }
        m_passing = m_stats.parsePassingScores(root);
        m_hasPassingCache = true;
        m_passingLastUpdated = QDateTime::currentDateTimeUtc();
        m_passingError.clear();
        writePassingCache(root);
        m_passingState = m_passing.groups.isEmpty() ? QueryState::Empty : QueryState::Loaded;
        emit passingChanged();
    });
}

// 清理内部状态或持久化数据。
void GradesViewModel::clearSchemeError()
{
    m_schemeError.clear();
    emit schemeChanged();
}

// 清理内部状态或持久化数据。
void GradesViewModel::clearPassingError()
{
    m_passingError.clear();
    emit passingChanged();
}

// 设置属性值并在变化时发出通知。
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

// 返回按搜索关键字过滤后的方案成绩分组。
QVariantList GradesViewModel::filteredSchemeGroups() const
{
    return termGroupsToVariant(groupGradeCoursesByTerm(filteredItems(m_scheme.items)));
}

// 返回按搜索关键字过滤后的及格成绩分组。
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

// 基于自定义课程集合计算统计信息。
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

// 清理本模块缓存并重置相关状态。
void GradesViewModel::clearCache()
{
    const bool schemeRemoved = !m_cache || m_cache->remove(QString::fromLatin1(SchemeKey));
    const bool passingRemoved = !m_cache || m_cache->remove(QString::fromLatin1(PassingKey));

    bool notifyScheme = false;
    bool notifyPassing = false;
    if (schemeRemoved) {
        notifyScheme = m_hasSchemeCache || !m_scheme.items.isEmpty()
            || m_schemeState != QueryState::Idle || !m_schemeError.isEmpty()
            || m_schemeLastUpdated.isValid();
        m_scheme = {};
        m_hasSchemeCache = false;
        m_schemeState = QueryState::Idle;
        m_schemeError.clear();
        m_schemeLastUpdated = {};
    } else {
        notifyScheme = m_schemeState != QueryState::Error
            || m_schemeError != SchemeCacheRemovalMessage;
        m_schemeState = QueryState::Error;
        m_schemeError = SchemeCacheRemovalMessage;
    }

    if (passingRemoved) {
        notifyPassing = m_hasPassingCache || !m_passing.groups.isEmpty()
            || m_passingState != QueryState::Idle || !m_passingError.isEmpty()
            || m_passingLastUpdated.isValid();
        m_passing = {};
        m_hasPassingCache = false;
        m_passingState = QueryState::Idle;
        m_passingError.clear();
        m_passingLastUpdated = {};
    } else {
        notifyPassing = m_passingState != QueryState::Error
            || m_passingError != PassingCacheRemovalMessage;
        m_passingState = QueryState::Error;
        m_passingError = PassingCacheRemovalMessage;
    }

    if (notifyScheme) {
        emit schemeChanged();
    }
    if (notifyPassing) {
        emit passingChanged();
    }
    if (schemeRemoved && passingRemoved && !m_searchQuery.isEmpty()) {
        m_searchQuery.clear();
        emit searchQueryChanged();
    }
    if (!schemeRemoved || !passingRemoved) {
        emit toastRequested(CacheRemovalFailureMessage);
    }
}

// 读取方案成绩缓存并恢复统计状态。
void GradesViewModel::readSchemeCache()
{
    if (!m_cache) {
        return;
    }
    QueryCacheEntry entry;
    if (!m_cache->get(QString::fromLatin1(SchemeKey), &entry)) {
        if (!m_cache->lastError().isEmpty()) {
            if (loggedIn()) {
                if (m_hasSchemeCache) {
                    refreshSchemeScores();
                }
            } else {
                const bool notify = m_hasSchemeCache || !m_scheme.items.isEmpty()
                    || m_schemeLastUpdated.isValid()
                    || m_schemeState != QueryState::LoginRequired
                    || m_schemeError != SchemeCacheReadMessage;
                m_scheme = {};
                m_hasSchemeCache = false;
                m_schemeLastUpdated = {};
                m_schemeState = QueryState::LoginRequired;
                m_schemeError = SchemeCacheReadMessage;
                if (notify) {
                    emit schemeChanged();
                    emit toastRequested(SchemeCacheReadMessage);
                }
            }
        }
        return;
    }

    QJsonObject root;
    if (!parseGradeCachePayload(entry.payloadJson, &root)) {
        const bool removed = m_cache->remove(QString::fromLatin1(SchemeKey));
        const bool canRefresh = loggedIn();
        const QString diagnostic = !removed ? DamagedSchemeRemovalMessage
            : (!canRefresh ? DamagedSchemeCacheMessage : QString());
        const bool notify = m_hasSchemeCache || !m_scheme.items.isEmpty()
            || (!canRefresh && m_schemeState != QueryState::LoginRequired)
            || m_schemeLastUpdated.isValid()
            || m_schemeError != diagnostic;
        m_scheme = {};
        m_hasSchemeCache = false;
        if (!canRefresh) {
            m_schemeState = QueryState::LoginRequired;
        }
        m_schemeLastUpdated = {};
        m_schemeError = diagnostic;
        if (!canRefresh && notify) {
            emit schemeChanged();
        }
        if (!diagnostic.isEmpty()) {
            emit toastRequested(diagnostic);
        }
        return;
    }

    m_scheme = m_stats.parseSchemeScores(root);
    m_schemeLastUpdated = entry.updatedAt;
    m_hasSchemeCache = true;
    m_schemeState = m_scheme.items.isEmpty() ? QueryState::Empty : QueryState::Loaded;
    emit schemeChanged();
}

// 读取及格成绩缓存并恢复统计状态。
void GradesViewModel::readPassingCache()
{
    if (!m_cache) {
        return;
    }
    QueryCacheEntry entry;
    if (!m_cache->get(QString::fromLatin1(PassingKey), &entry)) {
        if (!m_cache->lastError().isEmpty()) {
            if (loggedIn()) {
                if (m_hasPassingCache) {
                    refreshPassingScores();
                }
            } else {
                const bool notify = m_hasPassingCache || !m_passing.groups.isEmpty()
                    || m_passingLastUpdated.isValid()
                    || m_passingState != QueryState::LoginRequired
                    || m_passingError != PassingCacheReadMessage;
                m_passing = {};
                m_hasPassingCache = false;
                m_passingLastUpdated = {};
                m_passingState = QueryState::LoginRequired;
                m_passingError = PassingCacheReadMessage;
                if (notify) {
                    emit passingChanged();
                    emit toastRequested(PassingCacheReadMessage);
                }
            }
        }
        return;
    }

    QJsonObject root;
    if (!parseGradeCachePayload(entry.payloadJson, &root)) {
        const bool removed = m_cache->remove(QString::fromLatin1(PassingKey));
        const bool canRefresh = loggedIn();
        const QString diagnostic = !removed ? DamagedPassingRemovalMessage
            : (!canRefresh ? DamagedPassingCacheMessage : QString());
        const bool notify = m_hasPassingCache || !m_passing.groups.isEmpty()
            || (!canRefresh && m_passingState != QueryState::LoginRequired)
            || m_passingLastUpdated.isValid()
            || m_passingError != diagnostic;
        m_passing = {};
        m_hasPassingCache = false;
        if (!canRefresh) {
            m_passingState = QueryState::LoginRequired;
        }
        m_passingLastUpdated = {};
        m_passingError = diagnostic;
        if (!canRefresh && notify) {
            emit passingChanged();
        }
        if (!diagnostic.isEmpty()) {
            emit toastRequested(diagnostic);
        }
        return;
    }

    m_passing = m_stats.parsePassingScores(root);
    m_passingLastUpdated = entry.updatedAt;
    m_hasPassingCache = true;
    m_passingState = m_passing.groups.isEmpty() ? QueryState::Empty : QueryState::Loaded;
    emit passingChanged();
}

// 写入方案成绩接口原始数据缓存。
void GradesViewModel::writeSchemeCache(const QJsonObject &root)
{
    if (m_cache) {
        m_cache->put(QString::fromLatin1(SchemeKey), QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact)));
    }
}

// 写入及格成绩接口原始数据缓存。
void GradesViewModel::writePassingCache(const QJsonObject &root)
{
    if (m_cache) {
        m_cache->put(QString::fromLatin1(PassingKey), QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact)));
    }
}

// 根据搜索关键字过滤课程列表。
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

// 按课程属性过滤课程列表。
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

// 返回按课程属性过滤后的方案成绩分组。
QVariantList GradesViewModel::filteredSchemeGroupsByAttr(const QString &attr) const
{
    const QList<GradeCourseItem> items = filterItemsByAttr(filteredItems(m_scheme.items), attr);
    return termGroupsToVariant(groupGradeCoursesByTerm(items));
}

// 返回按课程属性过滤后的及格成绩分组。
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

void GradesViewModel::handleSchemeError(const ApiError &error)
{
    m_schemeError = error.message;
    if (m_hasSchemeCache) {
        m_schemeState = m_scheme.items.isEmpty() ? QueryState::Empty : QueryState::Loaded;
        emit toastRequested(error.message);
    } else {
        m_schemeState = error.type == ApiErrorType::Unauthenticated || error.type == ApiErrorType::SessionExpired
            ? QueryState::LoginRequired
            : QueryState::Error;
    }
    emit schemeChanged();
}

void GradesViewModel::handlePassingError(const ApiError &error)
{
    m_passingError = error.message;
    if (m_hasPassingCache) {
        m_passingState = m_passing.groups.isEmpty() ? QueryState::Empty : QueryState::Loaded;
        emit toastRequested(error.message);
    } else {
        m_passingState = error.type == ApiErrorType::Unauthenticated || error.type == ApiErrorType::SessionExpired
            ? QueryState::LoginRequired
            : QueryState::Error;
    }
    emit passingChanged();
}
