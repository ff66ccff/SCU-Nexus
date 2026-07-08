#include "viewmodels/ExamPlanViewModel.h"

#include <QJsonArray>
#include <QJsonDocument>

namespace {
constexpr auto ExamPlanKey = "exam_plan.latest";
}

ExamPlanViewModel::ExamPlanViewModel(QueryCacheRepository *cache, MockZhjwApiService *api, QObject *parent)
    : QObject(parent),
      m_cache(cache),
      m_api(api)
{
    if (m_api) {
        connect(m_api, &MockZhjwApiService::loggedInChanged, this, &ExamPlanViewModel::authChanged);
    }
}

QueryState ExamPlanViewModel::state() const { return m_state; }
bool ExamPlanViewModel::loading() const { return m_state == QueryState::Loading; }
QString ExamPlanViewModel::errorMessage() const { return m_errorMessage; }
QDateTime ExamPlanViewModel::lastUpdated() const { return m_lastUpdated; }
bool ExamPlanViewModel::hasCache() const { return m_hasCache; }
int ExamPlanViewModel::count() const { return m_exams.size(); }
bool ExamPlanViewModel::loggedIn() const { return m_api && m_api->loggedIn(); }

QVariantList ExamPlanViewModel::exams() const
{
    QVariantList list;
    for (const ExamPlanItem &item : m_exams) {
        list.append(item.toVariant());
    }
    return list;
}

void ExamPlanViewModel::load()
{
    readCache();
    if (!loggedIn() && !m_hasCache) {
        setState(QueryState::LoginRequired);
        return;
    }
    setState(m_hasCache ? (m_exams.isEmpty() ? QueryState::Empty : QueryState::Loaded) : QueryState::Loading);
    if (loggedIn()) {
        refresh();
    }
}

void ExamPlanViewModel::refresh()
{
    if (!loggedIn()) {
        if (m_hasCache) {
            emit toastRequested(QStringLiteral("登录已失效，正在显示缓存考表。"));
            setState(m_exams.isEmpty() ? QueryState::Empty : QueryState::Loaded);
        } else {
            setState(QueryState::LoginRequired);
        }
        return;
    }

    setState(QueryState::Loading);
    QList<ExamPlanItem> items;
    for (const QJsonObject &object : m_api->fetchExamPlan()) {
        items.append(ExamPlanItem::fromJson(object));
    }
    sortExamPlanItems(&items);
    m_exams = items;
    m_hasCache = true;
    m_lastUpdated = QDateTime::currentDateTimeUtc();
    writeCache();
    emit examsChanged();
    emit cacheChanged();
    emit lastUpdatedChanged();
    setState(m_exams.isEmpty() ? QueryState::Empty : QueryState::Loaded);
}

void ExamPlanViewModel::clearCache()
{
    if (m_cache) {
        m_cache->remove(QString::fromLatin1(ExamPlanKey));
    }
    m_exams.clear();
    m_hasCache = false;
    emit examsChanged();
    emit cacheChanged();
    setState(QueryState::Idle);
}

QVariantMap ExamPlanViewModel::examAt(int index) const
{
    if (index < 0 || index >= m_exams.size()) {
        return {};
    }
    return m_exams.at(index).toVariant();
}

void ExamPlanViewModel::setState(QueryState state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    emit stateChanged();
}

void ExamPlanViewModel::setError(const QString &message)
{
    if (m_errorMessage == message) {
        return;
    }
    m_errorMessage = message;
    emit errorChanged();
}

void ExamPlanViewModel::readCache()
{
    if (!m_cache) {
        return;
    }
    QueryCacheEntry entry;
    if (!m_cache->get(QString::fromLatin1(ExamPlanKey), &entry)) {
        return;
    }
    m_exams = examPlanItemsFromJson(QJsonDocument::fromJson(entry.payloadJson.toUtf8()).array());
    m_lastUpdated = entry.updatedAt;
    m_hasCache = true;
    emit examsChanged();
    emit lastUpdatedChanged();
    emit cacheChanged();
}

void ExamPlanViewModel::writeCache()
{
    if (m_cache) {
        m_cache->put(QString::fromLatin1(ExamPlanKey), QString::fromUtf8(QJsonDocument(examPlanItemsToJson(m_exams)).toJson(QJsonDocument::Compact)));
    }
}
