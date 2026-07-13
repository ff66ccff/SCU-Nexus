#include "viewmodels/AcademicCalendarViewModel.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
constexpr auto EntriesKey = "academic_calendar.entries";
constexpr auto SelectedKey = "academic_calendar.selected_year";
constexpr auto ImagesKey = "academic_calendar.images";
}

// 构造对象并初始化依赖关系。
AcademicCalendarViewModel::AcademicCalendarViewModel(QueryCacheRepository *cache, QObject *parent)
    : QObject(parent),
      m_cache(cache)
{
    connect(&m_service, &AcademicCalendarService::entriesFetched, this, [this](const QList<AcademicCalendarEntry> &entries) {
        applyEntries(entries, true);
    });
    connect(&m_service, &AcademicCalendarService::detailFetched, this, [this](const AcademicCalendarDetail &detail) {
        applyDetail(detail, true);
    });
    connect(&m_service, &AcademicCalendarService::failed, this, [this](const ApiError &error) {
        if (m_hasCache) {
            setState(m_imageUrls.isEmpty() ? QueryState::Empty : QueryState::Loaded);
            emit toastRequested(error.message);
        } else {
            setError(error.message);
            setState(error.type == ApiErrorType::Unauthenticated
                             || error.type == ApiErrorType::SessionExpired
                         ? QueryState::LoginRequired
                         : QueryState::Error);
        }
    });
}

// 返回当前查询状态。
QueryState AcademicCalendarViewModel::state() const { return m_state; }
// 加载当前模块数据并同步界面状态。
bool AcademicCalendarViewModel::loading() const { return m_state == QueryState::Loading; }
// 返回当前错误提示文本。
QString AcademicCalendarViewModel::errorMessage() const { return m_errorMessage; }
// 返回数据最近更新时间。
QDateTime AcademicCalendarViewModel::lastUpdated() const { return m_lastUpdated; }
// 返回是否已有可用缓存。
bool AcademicCalendarViewModel::hasCache() const { return m_hasCache; }
// 返回当前选中项相关数据。
int AcademicCalendarViewModel::selectedIndex() const { return m_selectedIndex; }
// 返回当前校历详情图片地址列表。
QStringList AcademicCalendarViewModel::imageUrls() const { return m_imageUrls; }

// 返回校历条目列表的界面数据。
QVariantList AcademicCalendarViewModel::entries() const
{
    QVariantList list;
    for (const AcademicCalendarEntry &entry : m_entries) {
        list.append(entry.toVariant());
    }
    return list;
}

// 返回当前选中项相关数据。
QString AcademicCalendarViewModel::selectedTitle() const
{
    if (m_selectedIndex < 0 || m_selectedIndex >= m_entries.size()) {
        return {};
    }
    return m_entries.at(m_selectedIndex).title;
}

// 加载当前模块数据并同步界面状态。
void AcademicCalendarViewModel::load()
{
    readCache();
    if (m_hasCache) {
        setState(m_imageUrls.isEmpty() ? QueryState::Empty : QueryState::Loaded);
    } else {
        setState(QueryState::Loading);
    }
    reloadList();
}

// 刷新远端数据并更新缓存状态。
void AcademicCalendarViewModel::refresh()
{
    reloadList();
}

// 清理本模块缓存并重置相关状态。
void AcademicCalendarViewModel::clearCache()
{
    if (!m_cache) {
        return;
    }
    m_cache->remove(QString::fromLatin1(EntriesKey));
    m_cache->remove(QString::fromLatin1(SelectedKey));
    m_cache->remove(QString::fromLatin1(ImagesKey));
    m_hasCache = false;
    m_entries.clear();
    m_imageUrls.clear();
    m_selectedIndex = -1;
    emit cacheChanged();
    emit entriesChanged();
    emit selectedChanged();
    emit imageUrlsChanged();
    setState(QueryState::Idle);
}

// 切换当前校历条目并加载对应详情。
void AcademicCalendarViewModel::selectEntry(int index)
{
    if (index < 0 || index >= m_entries.size() || index == m_selectedIndex) {
        return;
    }
    m_selectedIndex = index;
    m_imageUrls.clear();
    emit selectedChanged();
    emit imageUrlsChanged();
    writeSelectedCache();
    reloadSelected();
}

// 加载当前模块数据并同步界面状态。
void AcademicCalendarViewModel::reloadList()
{
    setState(QueryState::Loading);
    m_service.fetchEntries();
}

// 加载当前模块数据并同步界面状态。
void AcademicCalendarViewModel::reloadSelected()
{
    if (m_selectedIndex < 0 || m_selectedIndex >= m_entries.size()) {
        setState(QueryState::Empty);
        return;
    }
    setState(QueryState::Loading);
    m_service.fetchDetail(m_entries.at(m_selectedIndex));
}

// 更新查询状态并通知界面刷新。
void AcademicCalendarViewModel::setState(QueryState state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    emit stateChanged();
}

// 更新错误信息并触发错误状态通知。
void AcademicCalendarViewModel::setError(const QString &message)
{
    if (m_errorMessage == message) {
        return;
    }
    m_errorMessage = message;
    emit errorChanged();
}

// 读取本地缓存并恢复视图模型状态。
void AcademicCalendarViewModel::readCache()
{
    if (!m_cache) {
        return;
    }

    QueryCacheEntry entry;
    if (m_cache->get(QString::fromLatin1(EntriesKey), &entry)) {
        m_entries = academicCalendarEntriesFromJson(QJsonDocument::fromJson(entry.payloadJson.toUtf8()).array());
        m_lastUpdated = entry.updatedAt;
        m_hasCache = true;
        emit entriesChanged();
        emit lastUpdatedChanged();
        emit cacheChanged();
    }

    if (m_cache->get(QString::fromLatin1(SelectedKey), &entry)) {
        const QString title = QJsonDocument::fromJson(entry.payloadJson.toUtf8()).object().value(QStringLiteral("title")).toString();
        m_selectedIndex = -1;
        for (int i = 0; i < m_entries.size(); ++i) {
            if (m_entries.at(i).title == title) {
                m_selectedIndex = i;
                break;
            }
        }
        emit selectedChanged();
    }

    if (m_cache->get(QString::fromLatin1(ImagesKey), &entry)) {
        m_imageUrls.clear();
        const QJsonArray array = QJsonDocument::fromJson(entry.payloadJson.toUtf8()).array();
        for (const QJsonValue &value : array) {
            m_imageUrls.append(value.toString());
        }
        m_lastUpdated = entry.updatedAt;
        emit imageUrlsChanged();
        emit lastUpdatedChanged();
    }
}

// 写入校历条目列表缓存。
void AcademicCalendarViewModel::writeEntriesCache()
{
    if (m_cache) {
        m_cache->put(QString::fromLatin1(EntriesKey), QString::fromUtf8(QJsonDocument(academicCalendarEntriesToJson(m_entries)).toJson(QJsonDocument::Compact)));
    }
}

// 写入当前选中的校历条目缓存。
void AcademicCalendarViewModel::writeSelectedCache()
{
    if (!m_cache || m_selectedIndex < 0 || m_selectedIndex >= m_entries.size()) {
        return;
    }
    m_cache->put(QString::fromLatin1(SelectedKey), QString::fromUtf8(QJsonDocument(QJsonObject{{QStringLiteral("title"), selectedTitle()}}).toJson(QJsonDocument::Compact)));
}

// 写入校历详情图片地址缓存。
void AcademicCalendarViewModel::writeImagesCache()
{
    if (!m_cache) {
        return;
    }
    QJsonArray array;
    for (const QString &url : m_imageUrls) {
        array.append(url);
    }
    m_cache->put(QString::fromLatin1(ImagesKey), QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact)));
}

// 应用样式或脱敏规则到目标内容。
void AcademicCalendarViewModel::applyEntries(const QList<AcademicCalendarEntry> &entries, bool fromNetwork)
{
    m_entries = entries;
    if (m_entries.isEmpty()) {
        const bool hadImages = !m_imageUrls.isEmpty();
        m_selectedIndex = -1;
        m_imageUrls.clear();
        if (fromNetwork) {
            m_hasCache = true;
            m_lastUpdated = QDateTime::currentDateTimeUtc();
            writeEntriesCache();
            writeImagesCache();
            emit cacheChanged();
            emit lastUpdatedChanged();
        }
        if (hadImages) {
            emit imageUrlsChanged();
        }
        setState(QueryState::Empty);
    } else {
        if (m_selectedIndex < 0 || m_selectedIndex >= m_entries.size()) {
            m_selectedIndex = 0;
        }
        if (fromNetwork) {
            writeEntriesCache();
            writeSelectedCache();
        }
        reloadSelected();
    }
    emit entriesChanged();
    emit selectedChanged();
}

// 应用样式或脱敏规则到目标内容。
void AcademicCalendarViewModel::applyDetail(const AcademicCalendarDetail &detail, bool fromNetwork)
{
    m_imageUrls = detail.imageUrls;
    m_hasCache = true;
    m_lastUpdated = QDateTime::currentDateTimeUtc();
    if (fromNetwork) {
        writeImagesCache();
    }
    emit imageUrlsChanged();
    emit cacheChanged();
    emit lastUpdatedChanged();
    setState(m_imageUrls.isEmpty() ? QueryState::Empty : QueryState::Loaded);
}
