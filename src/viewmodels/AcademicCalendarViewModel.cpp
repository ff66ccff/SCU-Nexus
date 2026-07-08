#include "viewmodels/AcademicCalendarViewModel.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
constexpr auto EntriesKey = "academic_calendar.entries";
constexpr auto SelectedKey = "academic_calendar.selected_year";
constexpr auto ImagesKey = "academic_calendar.images";
}

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
    connect(&m_service, &AcademicCalendarService::failed, this, [this](const QString &message) {
        if (m_hasCache) {
            setState(m_imageUrls.isEmpty() ? QueryState::Empty : QueryState::Loaded);
            emit toastRequested(message);
        } else {
            setError(message);
            setState(QueryState::Error);
        }
    });
}

QueryState AcademicCalendarViewModel::state() const { return m_state; }
bool AcademicCalendarViewModel::loading() const { return m_state == QueryState::Loading; }
QString AcademicCalendarViewModel::errorMessage() const { return m_errorMessage; }
QDateTime AcademicCalendarViewModel::lastUpdated() const { return m_lastUpdated; }
bool AcademicCalendarViewModel::hasCache() const { return m_hasCache; }
int AcademicCalendarViewModel::selectedIndex() const { return m_selectedIndex; }
QStringList AcademicCalendarViewModel::imageUrls() const { return m_imageUrls; }

QVariantList AcademicCalendarViewModel::entries() const
{
    QVariantList list;
    for (const AcademicCalendarEntry &entry : m_entries) {
        list.append(entry.toVariant());
    }
    return list;
}

QString AcademicCalendarViewModel::selectedTitle() const
{
    if (m_selectedIndex < 0 || m_selectedIndex >= m_entries.size()) {
        return {};
    }
    return m_entries.at(m_selectedIndex).title;
}

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

void AcademicCalendarViewModel::refresh()
{
    reloadList();
}

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

void AcademicCalendarViewModel::reloadList()
{
    setState(QueryState::Loading);
    m_service.fetchEntries();
}

void AcademicCalendarViewModel::reloadSelected()
{
    if (m_selectedIndex < 0 || m_selectedIndex >= m_entries.size()) {
        setState(QueryState::Empty);
        return;
    }
    setState(QueryState::Loading);
    m_service.fetchDetail(m_entries.at(m_selectedIndex));
}

void AcademicCalendarViewModel::setState(QueryState state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    emit stateChanged();
}

void AcademicCalendarViewModel::setError(const QString &message)
{
    if (m_errorMessage == message) {
        return;
    }
    m_errorMessage = message;
    emit errorChanged();
}

void AcademicCalendarViewModel::readCache()
{
    if (!m_cache) {
        return;
    }

    QueryCacheEntry entry;
    if (m_cache->get(QString::fromLatin1(EntriesKey), &entry)) {
        m_entries = academicCalendarEntriesFromJson(QJsonDocument::fromJson(entry.payloadJson.toUtf8()).array());
        m_lastUpdated = entry.updatedAt;
        m_hasCache = !m_entries.isEmpty();
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

void AcademicCalendarViewModel::writeEntriesCache()
{
    if (m_cache) {
        m_cache->put(QString::fromLatin1(EntriesKey), QString::fromUtf8(QJsonDocument(academicCalendarEntriesToJson(m_entries)).toJson(QJsonDocument::Compact)));
    }
}

void AcademicCalendarViewModel::writeSelectedCache()
{
    if (!m_cache || m_selectedIndex < 0 || m_selectedIndex >= m_entries.size()) {
        return;
    }
    m_cache->put(QString::fromLatin1(SelectedKey), QString::fromUtf8(QJsonDocument(QJsonObject{{QStringLiteral("title"), selectedTitle()}}).toJson(QJsonDocument::Compact)));
}

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

void AcademicCalendarViewModel::applyEntries(const QList<AcademicCalendarEntry> &entries, bool fromNetwork)
{
    m_entries = entries;
    if (m_entries.isEmpty()) {
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
