#include "viewmodels/AcademicCalendarViewModel.h"

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <utility>

namespace {
constexpr auto EntriesKey = "academic_calendar.entries";
constexpr auto SelectedKey = "academic_calendar.selected_year";
constexpr auto LegacyImagesKey = "academic_calendar.images";
const QString CacheWriteWarning = QStringLiteral("校历数据已更新，但缓存写入失败。");
const QString DamagedCacheRemovalWarning = QStringLiteral("移除损坏的校历缓存失败，请重试。");
const QString CacheRemovalWarning = QStringLiteral("清除校历缓存失败，请重试。");

bool nonEmptyString(const QJsonValue &value)
{
    return value.isString() && !value.toString().trimmed().isEmpty();
}

bool parseEntriesCache(const QString &payload, QList<AcademicCalendarEntry> *entries)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isArray()) {
        return false;
    }

    QList<AcademicCalendarEntry> parsed;
    for (const QJsonValue &value : document.array()) {
        if (!value.isObject()) {
            return false;
        }
        const QJsonObject object = value.toObject();
        if (!nonEmptyString(object.value(QStringLiteral("title")))
            || !nonEmptyString(object.value(QStringLiteral("path")))) {
            return false;
        }
        parsed.append(AcademicCalendarEntry::fromJson(object));
    }
    *entries = parsed;
    return true;
}

enum class SelectedCacheShape {
    Invalid,
    Current,
    LegacyTitleOnly
};

SelectedCacheShape parseSelectedCache(const QString &payload,
                                      QString *path,
                                      QString *title)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return SelectedCacheShape::Invalid;
    }
    const QJsonObject object = document.object();
    if (!nonEmptyString(object.value(QStringLiteral("title")))) {
        return SelectedCacheShape::Invalid;
    }
    *title = object.value(QStringLiteral("title")).toString();
    if (!object.contains(QStringLiteral("path"))) {
        path->clear();
        return SelectedCacheShape::LegacyTitleOnly;
    }
    if (!nonEmptyString(object.value(QStringLiteral("path")))) {
        return SelectedCacheShape::Invalid;
    }
    *path = object.value(QStringLiteral("path")).toString();
    return SelectedCacheShape::Current;
}

bool parseImagesCache(const QString &payload, QStringList *images)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isArray()) {
        return false;
    }

    QStringList parsed;
    for (const QJsonValue &value : document.array()) {
        if (!nonEmptyString(value)) {
            return false;
        }
        parsed.append(value.toString());
    }
    *images = parsed;
    return true;
}

int indexByPath(const QList<AcademicCalendarEntry> &entries, const QString &path)
{
    if (path.isEmpty()) {
        return -1;
    }
    for (int i = 0; i < entries.size(); ++i) {
        if (entries.at(i).path == path) {
            return i;
        }
    }
    return -1;
}

int indexByTitle(const QList<AcademicCalendarEntry> &entries, const QString &title)
{
    if (title.isEmpty()) {
        return -1;
    }
    for (int i = 0; i < entries.size(); ++i) {
        if (entries.at(i).title == title) {
            return i;
        }
    }
    return -1;
}

int uniqueIndexByTitle(const QList<AcademicCalendarEntry> &entries, const QString &title)
{
    int match = -1;
    for (int i = 0; i < entries.size(); ++i) {
        if (entries.at(i).title != title) {
            continue;
        }
        if (match >= 0) {
            return -1;
        }
        match = i;
    }
    return match;
}
}

// 构造对象并初始化依赖关系。
AcademicCalendarViewModel::AcademicCalendarViewModel(QueryCacheRepository *cache, QObject *parent)
    : AcademicCalendarViewModel(cache, nullptr, parent)
{}

AcademicCalendarViewModel::AcademicCalendarViewModel(QueryCacheRepository *cache,
                                                     AcademicCalendarService *service,
                                                     QObject *parent)
    : QObject(parent),
      m_cache(cache),
      m_activeService(service ? service : &m_service)
{
    connect(m_activeService, &AcademicCalendarService::entriesFetched, this, [this](const QList<AcademicCalendarEntry> &entries) {
        m_pendingRequest = PendingRequest::None;
        applyEntries(entries, true);
    });
    connect(m_activeService, &AcademicCalendarService::detailFetched, this, [this](const AcademicCalendarDetail &detail) {
        if (m_selectedIndex < 0 || m_selectedIndex >= m_entries.size()
            || detail.entry.path != m_entries.at(m_selectedIndex).path) {
            return;
        }
        m_pendingRequest = PendingRequest::None;
        applyDetail(detail, true);
    });
    connect(m_activeService, &AcademicCalendarService::failed, this, [this](const ApiError &error) {
        const PendingRequest failedRequest = m_pendingRequest;
        const bool preserve = m_preserveOnFailure
            || (failedRequest == PendingRequest::None && m_hasCache);
        const QueryState restoreState = m_restoreState;
        m_pendingRequest = PendingRequest::None;
        m_preserveOnFailure = false;
        if (preserve) {
            if (restoreState == QueryState::Loaded || restoreState == QueryState::Empty) {
                setState(restoreState);
            } else {
                setState(m_imageUrls.isEmpty() ? QueryState::Empty : QueryState::Loaded);
            }
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

QString AcademicCalendarViewModel::imagesCacheKey(const QString &path)
{
    return QStringLiteral("academic_calendar.images.%1")
        .arg(QString::fromLatin1(
            QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Sha256).toHex()));
}

// 返回当前查询状态。
QueryState AcademicCalendarViewModel::state() const { return m_state; }
// 加载当前模块数据并同步界面状态。
bool AcademicCalendarViewModel::loading() const
{
    return m_state == QueryState::Loading || m_state == QueryState::Refreshing;
}
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
    m_activeService->invalidatePending();
    m_pendingRequest = PendingRequest::None;
    m_preserveOnFailure = false;

    for (const AcademicCalendarEntry &entry : std::as_const(m_entries)) {
        if (!entry.path.isEmpty()) {
            m_knownEntryPaths.insert(entry.path);
        }
    }
    if (!m_imageEntryPath.isEmpty()) {
        m_knownEntryPaths.insert(m_imageEntryPath);
    }

    QSet<QString> keys{
        QString::fromLatin1(EntriesKey),
        QString::fromLatin1(SelectedKey),
        QString::fromLatin1(LegacyImagesKey)
    };
    for (const QString &path : std::as_const(m_knownEntryPaths)) {
        keys.insert(imagesCacheKey(path));
    }

    bool allRemoved = true;
    QSet<QString> removedKeys;
    if (m_cache) {
        for (const QString &key : std::as_const(keys)) {
            if (m_cache->remove(key)) {
                removedKeys.insert(key);
            } else {
                allRemoved = false;
            }
        }
    } else {
        removedKeys = keys;
    }

    if (removedKeys.contains(QString::fromLatin1(EntriesKey))) {
        m_hasEntriesCache = false;
    }
    if (!m_persistedImageKey.isEmpty() && removedKeys.contains(m_persistedImageKey)) {
        m_hasImagesCache = false;
        m_persistedImageKey.clear();
    }
    syncHasCache();

    if (!allRemoved) {
        emit toastRequested(CacheRemovalWarning);
        if (m_state != QueryState::Loaded && m_state != QueryState::Empty) {
            setState(m_imageUrls.isEmpty() ? QueryState::Empty : QueryState::Loaded);
        }
        return;
    }

    const bool entriesDidChange = !m_entries.isEmpty();
    const bool selectionDidChange = m_selectedIndex != -1;
    const bool imagesDidChange = !m_imageUrls.isEmpty();
    const bool updatedChanged = m_lastUpdated.isValid();
    m_entries.clear();
    m_imageUrls.clear();
    m_selectedIndex = -1;
    m_imageEntryPath.clear();
    m_persistedImageKey.clear();
    m_knownEntryPaths.clear();
    m_hasEntriesCache = false;
    m_hasImagesCache = false;
    syncHasCache();
    m_lastUpdated = {};
    m_entriesUpdatedAt = {};
    if (entriesDidChange) {
        emit entriesChanged();
    }
    if (selectionDidChange) {
        emit selectedChanged();
    }
    if (imagesDidChange) {
        emit imageUrlsChanged();
    }
    if (updatedChanged) {
        emit lastUpdatedChanged();
    }
    setError(QString());
    setState(QueryState::Idle);
}

// 切换当前校历条目并加载对应详情。
void AcademicCalendarViewModel::selectEntry(int index)
{
    if (index < 0 || index >= m_entries.size() || index == m_selectedIndex) {
        return;
    }
    m_selectedIndex = index;
    emit selectedChanged();
    if (m_cache && !writeSelectedCache()) {
        emit toastRequested(CacheWriteWarning);
    }
    readSelectedImagesCache(false);
    m_pendingRequest = PendingRequest::None;
    m_preserveOnFailure = false;
    setState(m_imageUrls.isEmpty() ? QueryState::Empty : QueryState::Loaded);
    reloadSelected();
}

// 加载当前模块数据并同步界面状态。
void AcademicCalendarViewModel::reloadList()
{
    if (m_state == QueryState::Loading || m_state == QueryState::Refreshing) {
        return;
    }
    m_restoreState = m_state == QueryState::Loaded || m_state == QueryState::Empty
        ? m_state
        : (m_imageUrls.isEmpty() ? QueryState::Empty : QueryState::Loaded);
    m_preserveOnFailure = m_hasEntriesCache || !m_entries.isEmpty();
    m_pendingRequest = PendingRequest::Entries;
    setState(m_preserveOnFailure ? QueryState::Refreshing : QueryState::Loading);
    m_activeService->fetchEntries();
}

// 加载当前模块数据并同步界面状态。
void AcademicCalendarViewModel::reloadSelected()
{
    if (m_selectedIndex < 0 || m_selectedIndex >= m_entries.size()) {
        setState(QueryState::Empty);
        return;
    }
    if (m_state == QueryState::Loading || m_state == QueryState::Refreshing) {
        return;
    }
    m_restoreState = m_state == QueryState::Loaded || m_state == QueryState::Empty
        ? m_state
        : (m_imageUrls.isEmpty() ? QueryState::Empty : QueryState::Loaded);
    m_preserveOnFailure = m_hasImagesCache || !m_entries.isEmpty();
    m_pendingRequest = PendingRequest::Detail;
    setState(m_preserveOnFailure ? QueryState::Refreshing : QueryState::Loading);
    m_activeService->fetchDetail(m_entries.at(m_selectedIndex));
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
        QList<AcademicCalendarEntry> cachedEntries;
        if (!parseEntriesCache(entry.payloadJson, &cachedEntries)) {
            const bool updatedDidChange = m_lastUpdated.isValid();
            if (!m_cache->remove(QString::fromLatin1(EntriesKey))) {
                emit toastRequested(DamagedCacheRemovalWarning);
            }
            m_entries.clear();
            m_hasEntriesCache = false;
            m_selectedIndex = -1;
            m_imageUrls.clear();
            m_imageEntryPath.clear();
            m_hasImagesCache = false;
            m_lastUpdated = {};
            m_entriesUpdatedAt = {};
            syncHasCache();
            emit entriesChanged();
            emit selectedChanged();
            emit imageUrlsChanged();
            if (updatedDidChange) {
                emit lastUpdatedChanged();
            }
        } else {
            m_entries = cachedEntries;
            for (const AcademicCalendarEntry &cachedEntry : std::as_const(m_entries)) {
                m_knownEntryPaths.insert(cachedEntry.path);
            }
            m_lastUpdated = entry.updatedAt;
            m_entriesUpdatedAt = entry.updatedAt;
            m_hasEntriesCache = true;
            syncHasCache();
            emit entriesChanged();
            emit lastUpdatedChanged();
        }
    }

    bool selectionSafelyAssociated = false;
    bool migrateLegacySelection = false;
    if (m_cache->get(QString::fromLatin1(SelectedKey), &entry)) {
        QString path;
        QString title;
        const SelectedCacheShape shape = parseSelectedCache(entry.payloadJson, &path, &title);
        if (shape == SelectedCacheShape::Invalid) {
            if (!m_cache->remove(QString::fromLatin1(SelectedKey))) {
                emit toastRequested(DamagedCacheRemovalWarning);
            }
        } else if (shape == SelectedCacheShape::Current) {
            m_knownEntryPaths.insert(path);
            m_selectedIndex = indexByPath(m_entries, path);
            if (m_selectedIndex >= 0) {
                selectionSafelyAssociated = true;
            } else {
                m_selectedIndex = indexByTitle(m_entries, title);
                selectionSafelyAssociated = m_selectedIndex >= 0
                    && uniqueIndexByTitle(m_entries, title) == m_selectedIndex;
            }
        } else {
            m_selectedIndex = uniqueIndexByTitle(m_entries, title);
            selectionSafelyAssociated = m_selectedIndex >= 0;
            migrateLegacySelection = selectionSafelyAssociated;
            if (!selectionSafelyAssociated
                && !m_cache->remove(QString::fromLatin1(SelectedKey))) {
                emit toastRequested(DamagedCacheRemovalWarning);
            }
        }
    }
    if (m_selectedIndex < 0 && !m_entries.isEmpty()) {
        m_selectedIndex = 0;
    }
    emit selectedChanged();
    if (migrateLegacySelection) {
        if (!writeSelectedCache()) {
            emit toastRequested(CacheWriteWarning);
        }
    }

    readSelectedImagesCache(true, selectionSafelyAssociated);
}

// 写入校历条目列表缓存。
bool AcademicCalendarViewModel::writeEntriesCache()
{
    if (!m_cache) {
        return false;
    }
    return m_cache->put(
        QString::fromLatin1(EntriesKey),
        QString::fromUtf8(
            QJsonDocument(academicCalendarEntriesToJson(m_entries)).toJson(QJsonDocument::Compact)),
        m_entriesUpdatedAt);
}

// 写入当前选中的校历条目缓存。
bool AcademicCalendarViewModel::writeSelectedCache()
{
    if (!m_cache || m_selectedIndex < 0 || m_selectedIndex >= m_entries.size()) {
        return false;
    }
    const AcademicCalendarEntry &selected = m_entries.at(m_selectedIndex);
    return m_cache->put(
        QString::fromLatin1(SelectedKey),
        QString::fromUtf8(QJsonDocument(QJsonObject{
            {QStringLiteral("path"), selected.path},
            {QStringLiteral("title"), selected.title}
        }).toJson(QJsonDocument::Compact)));
}

// 写入校历详情图片地址缓存。
bool AcademicCalendarViewModel::writeImagesCache()
{
    QString path = m_imageEntryPath;
    if (path.isEmpty() && m_selectedIndex >= 0 && m_selectedIndex < m_entries.size()) {
        path = m_entries.at(m_selectedIndex).path;
    }
    if (!m_cache || path.isEmpty()) {
        return false;
    }
    QJsonArray array;
    for (const QString &url : m_imageUrls) {
        array.append(url);
    }
    return m_cache->put(
        imagesCacheKey(path),
        QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact)));
}

bool AcademicCalendarViewModel::readSelectedImagesCache(bool allowLegacyMigration,
                                                        bool selectionSafelyAssociated)
{
    const QString previousPath = m_imageEntryPath;
    const QStringList previousImages = m_imageUrls;
    const QDateTime previousUpdated = m_lastUpdated;
    m_lastUpdated = m_entriesUpdatedAt;
    m_imageUrls.clear();
    m_hasImagesCache = false;
    m_persistedImageKey.clear();
    m_imageEntryPath = m_selectedIndex >= 0 && m_selectedIndex < m_entries.size()
        ? m_entries.at(m_selectedIndex).path
        : QString();
    if (!m_imageEntryPath.isEmpty()) {
        m_knownEntryPaths.insert(m_imageEntryPath);
    }

    QueryCacheEntry entry;
    bool loaded = false;
    if (m_cache && !m_imageEntryPath.isEmpty()
        && m_cache->get(imagesCacheKey(m_imageEntryPath), &entry)) {
        QStringList cachedImages;
        if (parseImagesCache(entry.payloadJson, &cachedImages)) {
            m_imageUrls = cachedImages;
            m_lastUpdated = entry.updatedAt;
            m_hasImagesCache = true;
            m_persistedImageKey = imagesCacheKey(m_imageEntryPath);
            loaded = true;
        } else {
            if (!m_cache->remove(imagesCacheKey(m_imageEntryPath))) {
                emit toastRequested(DamagedCacheRemovalWarning);
            }
        }
    }

    if (allowLegacyMigration && m_cache
        && m_cache->get(QString::fromLatin1(LegacyImagesKey), &entry)) {
        QStringList legacyImages;
        const bool validLegacy = parseImagesCache(entry.payloadJson, &legacyImages);
        if (validLegacy && selectionSafelyAssociated && !m_imageEntryPath.isEmpty()) {
            if (!loaded) {
                m_imageUrls = legacyImages;
                m_lastUpdated = entry.updatedAt;
                m_hasImagesCache = true;
                m_persistedImageKey = QString::fromLatin1(LegacyImagesKey);
                loaded = true;
                if (m_cache->put(imagesCacheKey(m_imageEntryPath),
                                 entry.payloadJson,
                                 entry.updatedAt)) {
                    m_persistedImageKey = imagesCacheKey(m_imageEntryPath);
                    if (!m_cache->remove(QString::fromLatin1(LegacyImagesKey))) {
                        emit toastRequested(DamagedCacheRemovalWarning);
                    }
                } else {
                    emit toastRequested(CacheWriteWarning);
                }
            } else {
                if (!m_cache->remove(QString::fromLatin1(LegacyImagesKey))) {
                    emit toastRequested(DamagedCacheRemovalWarning);
                }
            }
        } else {
            if (!m_cache->remove(QString::fromLatin1(LegacyImagesKey))) {
                emit toastRequested(DamagedCacheRemovalWarning);
            }
        }
    }

    syncHasCache();
    if (previousPath != m_imageEntryPath || previousImages != m_imageUrls) {
        emit imageUrlsChanged();
    }
    if (previousUpdated != m_lastUpdated) {
        emit lastUpdatedChanged();
    }
    return loaded;
}

void AcademicCalendarViewModel::syncHasCache()
{
    const bool hasCacheNow = m_hasEntriesCache || m_hasImagesCache;
    if (m_hasCache == hasCacheNow) {
        return;
    }
    m_hasCache = hasCacheNow;
    emit cacheChanged();
}

// 应用样式或脱敏规则到目标内容。
void AcademicCalendarViewModel::applyEntries(const QList<AcademicCalendarEntry> &entries, bool fromNetwork)
{
    m_pendingRequest = PendingRequest::None;
    m_preserveOnFailure = false;
    const QString previousPath = m_selectedIndex >= 0 && m_selectedIndex < m_entries.size()
        ? m_entries.at(m_selectedIndex).path
        : QString();
    const QString previousTitle = m_selectedIndex >= 0 && m_selectedIndex < m_entries.size()
        ? m_entries.at(m_selectedIndex).title
        : QString();
    for (const AcademicCalendarEntry &knownEntry : std::as_const(m_entries)) {
        if (!knownEntry.path.isEmpty()) {
            m_knownEntryPaths.insert(knownEntry.path);
        }
    }
    m_entries = entries;
    for (const AcademicCalendarEntry &knownEntry : std::as_const(m_entries)) {
        if (!knownEntry.path.isEmpty()) {
            m_knownEntryPaths.insert(knownEntry.path);
        }
    }
    if (fromNetwork) {
        m_entriesUpdatedAt = QDateTime::currentDateTimeUtc();
    }
    if (m_entries.isEmpty()) {
        const bool hadImages = !m_imageUrls.isEmpty();
        m_selectedIndex = -1;
        m_imageUrls.clear();
        m_imageEntryPath = previousPath;
        if (fromNetwork) {
            m_lastUpdated = m_entriesUpdatedAt;
            m_hasEntriesCache = writeEntriesCache();
            const bool shouldStoreImages = !m_imageEntryPath.isEmpty();
            m_hasImagesCache = shouldStoreImages && writeImagesCache();
            m_persistedImageKey = m_hasImagesCache
                ? imagesCacheKey(m_imageEntryPath)
                : QString();
            syncHasCache();
            if (m_cache && (!m_hasEntriesCache
                            || (shouldStoreImages && !m_hasImagesCache))) {
                emit toastRequested(CacheWriteWarning);
            }
            emit lastUpdatedChanged();
        }
        if (hadImages) {
            emit imageUrlsChanged();
        }
        setState(QueryState::Empty);
    } else {
        m_selectedIndex = indexByPath(m_entries, previousPath);
        if (m_selectedIndex < 0) {
            m_selectedIndex = indexByTitle(m_entries, previousTitle);
        }
        if (m_selectedIndex < 0) {
            m_selectedIndex = 0;
        }
        if (fromNetwork) {
            m_hasEntriesCache = writeEntriesCache();
            const bool selectedStored = writeSelectedCache();
            syncHasCache();
            if (m_cache && (!m_hasEntriesCache || !selectedStored)) {
                emit toastRequested(CacheWriteWarning);
            }
        }
        if (m_imageEntryPath != m_entries.at(m_selectedIndex).path) {
            readSelectedImagesCache(false);
        }
        if (m_state == QueryState::Loading || m_state == QueryState::Refreshing) {
            setState(m_imageUrls.isEmpty() ? QueryState::Empty : QueryState::Loaded);
        }
        reloadSelected();
    }
    emit entriesChanged();
    emit selectedChanged();
}

// 应用样式或脱敏规则到目标内容。
void AcademicCalendarViewModel::applyDetail(const AcademicCalendarDetail &detail, bool fromNetwork)
{
    m_pendingRequest = PendingRequest::None;
    m_preserveOnFailure = false;
    m_imageUrls = detail.imageUrls;
    m_imageEntryPath = detail.entry.path;
    m_knownEntryPaths.insert(detail.entry.path);
    m_lastUpdated = QDateTime::currentDateTimeUtc();
    if (fromNetwork) {
        m_hasImagesCache = writeImagesCache();
        m_persistedImageKey = m_hasImagesCache
            ? imagesCacheKey(m_imageEntryPath)
            : QString();
        if (m_cache && !m_hasImagesCache) {
            emit toastRequested(CacheWriteWarning);
        }
    }
    emit imageUrlsChanged();
    syncHasCache();
    emit lastUpdatedChanged();
    setState(m_imageUrls.isEmpty() ? QueryState::Empty : QueryState::Loaded);
}
