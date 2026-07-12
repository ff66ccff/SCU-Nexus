#include "viewmodels/ExamPlanViewModel.h"

#include <QJsonArray>
#include <QJsonDocument>

namespace {
constexpr auto ExamPlanKey = "exam_plan.latest";
const QString DamagedCacheMessage = QStringLiteral("考试缓存已损坏，已移除。");
const QString DamagedCacheRemovalMessage = QStringLiteral("移除损坏的考试缓存失败，请重试。");
const QString CacheReadMessage = QStringLiteral("读取考试缓存失败，请重试。");
const QString CacheRemovalMessage = QStringLiteral("清除考试缓存失败，请重试。");
}

// 构造对象并初始化依赖关系。
ExamPlanViewModel::ExamPlanViewModel(QueryCacheRepository *cache, ZhjwQueryService *api, QObject *parent)
    : QObject(parent),
      m_cache(cache),
      m_api(api)
{
    if (m_api) {
        connect(m_api, &ZhjwQueryService::loggedInChanged, this, &ExamPlanViewModel::authChanged);
    }
}

// 返回当前查询状态。
QueryState ExamPlanViewModel::state() const { return m_state; }
// 加载当前模块数据并同步界面状态。
bool ExamPlanViewModel::loading() const { return m_state == QueryState::Loading; }
// 返回当前错误提示文本。
QString ExamPlanViewModel::errorMessage() const { return m_errorMessage; }
// 返回数据最近更新时间。
QDateTime ExamPlanViewModel::lastUpdated() const { return m_lastUpdated; }
// 返回是否已有可用缓存。
bool ExamPlanViewModel::hasCache() const { return m_hasCache; }
// 返回当前数据条目数量。
int ExamPlanViewModel::count() const { return m_exams.size(); }
// 返回当前登录状态。
bool ExamPlanViewModel::loggedIn() const { return m_api && m_api->loggedIn(); }

// 返回考试安排列表的界面数据。
QVariantList ExamPlanViewModel::exams() const
{
    QVariantList list;
    for (const ExamPlanItem &item : m_exams) {
        list.append(item.toVariant());
    }
    return list;
}

// 加载当前模块数据并同步界面状态。
void ExamPlanViewModel::load()
{
    readCache();
    if (loggedIn()) {
        if (m_hasCache) {
            setState(m_exams.isEmpty() ? QueryState::Empty : QueryState::Loaded);
        }
        refresh();
        return;
    }
    setState(m_hasCache ? (m_exams.isEmpty() ? QueryState::Empty : QueryState::Loaded) : QueryState::LoginRequired);
}

// 刷新远端数据并更新缓存状态。
void ExamPlanViewModel::refresh()
{
    if (m_state == QueryState::Loading) {
        return;
    }
    if (!loggedIn()) {
        if (m_hasCache) {
            emit toastRequested(QStringLiteral("登录已失效，正在显示缓存考表。"));
            setState(m_exams.isEmpty() ? QueryState::Empty : QueryState::Loaded);
        } else {
            setState(QueryState::LoginRequired);
        }
        return;
    }

    setError(QString());
    setState(QueryState::Loading);
    m_api->fetchExamPlan([this](const QList<ExamPlanItemDto> &dtos, const ApiError &error) {
        if (error.type != ApiErrorType::Unknown) {
            setError(error.message);
            if (m_hasCache) {
                emit toastRequested(error.message);
                setState(m_exams.isEmpty() ? QueryState::Empty : QueryState::Loaded);
            } else {
                setState(error.type == ApiErrorType::Unauthenticated || error.type == ApiErrorType::SessionExpired
                    ? QueryState::LoginRequired
                    : QueryState::Error);
            }
            return;
        }

        QList<ExamPlanItem> items;
        for (const ExamPlanItemDto &dto : dtos) {
            items.append(fromDto(dto));
        }
        sortExamPlanItems(&items);
        m_exams = items;
        m_hasCache = true;
        m_lastUpdated = QDateTime::currentDateTimeUtc();
        setError(QString());
        writeCache();
        emit examsChanged();
        emit cacheChanged();
        emit lastUpdatedChanged();
        setState(m_exams.isEmpty() ? QueryState::Empty : QueryState::Loaded);
    });
}

// 清理本模块缓存并重置相关状态。
void ExamPlanViewModel::clearCache()
{
    if (m_cache && !m_cache->remove(QString::fromLatin1(ExamPlanKey))) {
        setError(CacheRemovalMessage);
        setState(QueryState::Error);
        return;
    }

    const bool itemsDidChange = !m_exams.isEmpty();
    const bool cacheDidChange = m_hasCache;
    const bool updatedDidChange = m_lastUpdated.isValid();
    m_exams.clear();
    m_hasCache = false;
    m_lastUpdated = {};
    if (itemsDidChange) {
        emit examsChanged();
    }
    if (cacheDidChange) {
        emit cacheChanged();
    }
    if (updatedDidChange) {
        emit lastUpdatedChanged();
    }
    setError(QString());
    setState(QueryState::Idle);
}

// 按索引返回单条考试安排数据。
QVariantMap ExamPlanViewModel::examAt(int index) const
{
    if (index < 0 || index >= m_exams.size()) {
        return {};
    }
    return m_exams.at(index).toVariant();
}

ExamPlanItem ExamPlanViewModel::fromDto(const ExamPlanItemDto &dto) const
{
    ExamPlanItem item;
    item.courseName = dto.courseName;
    item.week = dto.week;
    item.date = dto.date;
    item.weekday = dto.weekday;
    item.timeRange = dto.timeRange;
    item.location = dto.location;
    item.seatNumber = dto.seatNumber;
    item.ticketNumber = dto.ticketNumber;
    item.tip = dto.tip;
    updateExamTemporalFields(&item);
    return item;
}

// 更新查询状态并通知界面刷新。
void ExamPlanViewModel::setState(QueryState state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    emit stateChanged();
}

// 更新错误信息并触发错误状态通知。
void ExamPlanViewModel::setError(const QString &message)
{
    if (m_errorMessage == message) {
        return;
    }
    m_errorMessage = message;
    emit errorChanged();
}

// 读取本地缓存并恢复视图模型状态。
void ExamPlanViewModel::readCache()
{
    if (!m_cache) {
        return;
    }
    QueryCacheEntry entry;
    if (!m_cache->get(QString::fromLatin1(ExamPlanKey), &entry)) {
        if (!m_cache->lastError().isEmpty() && !loggedIn()) {
            const bool itemsDidChange = !m_exams.isEmpty();
            const bool cacheDidChange = m_hasCache;
            const bool updatedDidChange = m_lastUpdated.isValid();
            const bool notify = itemsDidChange || cacheDidChange || updatedDidChange
                || m_state != QueryState::LoginRequired || m_errorMessage != CacheReadMessage;
            m_exams.clear();
            m_hasCache = false;
            m_lastUpdated = {};
            if (itemsDidChange) {
                emit examsChanged();
            }
            if (cacheDidChange) {
                emit cacheChanged();
            }
            if (updatedDidChange) {
                emit lastUpdatedChanged();
            }
            setError(CacheReadMessage);
            setState(QueryState::LoginRequired);
            if (notify) {
                emit toastRequested(CacheReadMessage);
            }
        }
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(entry.payloadJson.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isArray()) {
        const bool removed = m_cache->remove(QString::fromLatin1(ExamPlanKey));
        const bool itemsDidChange = !m_exams.isEmpty();
        const bool cacheDidChange = m_hasCache;
        const bool updatedDidChange = m_lastUpdated.isValid();
        m_exams.clear();
        m_hasCache = false;
        m_lastUpdated = {};
        if (itemsDidChange) {
            emit examsChanged();
        }
        if (cacheDidChange) {
            emit cacheChanged();
        }
        if (updatedDidChange) {
            emit lastUpdatedChanged();
        }
        const QString diagnostic = !removed ? DamagedCacheRemovalMessage
            : (!loggedIn() ? DamagedCacheMessage : QString());
        setError(diagnostic);
        if (!diagnostic.isEmpty()) {
            emit toastRequested(diagnostic);
        }
        return;
    }

    m_exams = examPlanItemsFromJson(document.array());
    m_lastUpdated = entry.updatedAt;
    m_hasCache = true;
    emit examsChanged();
    emit lastUpdatedChanged();
    emit cacheChanged();
}

// 写入本地缓存以便后续离线展示。
void ExamPlanViewModel::writeCache()
{
    if (m_cache) {
        m_cache->put(QString::fromLatin1(ExamPlanKey), QString::fromUtf8(QJsonDocument(examPlanItemsToJson(m_exams)).toJson(QJsonDocument::Compact)));
    }
}
