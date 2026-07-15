#include "src/services/zhjw/ZhjwQueryService.h"

#include "src/services/api/ZhjwApiService.h"

ZhjwApiQueryService::ZhjwApiQueryService(QObject *parent, ZhjwApiService *api)
    : ZhjwQueryService(parent)
    , m_api(api)
{
    if (!m_api) {
        m_api = new ZhjwApiService(this);
        m_ownsApi = true;
    }
}

bool ZhjwApiQueryService::loggedIn() const
{
    return m_loggedIn;
}

void ZhjwApiQueryService::setLoggedIn(bool loggedIn)
{
    if (m_loggedIn == loggedIn) {
        return;
    }
    m_loggedIn = loggedIn;
    emit loggedInChanged();
}

void ZhjwApiQueryService::fetchExamPlan(ExamPlanCallback callback)
{
    // 适配层不排序、不缓存、不改 DTO，查询 ViewModel 才拥有这些页面状态。
    if (!m_api) {
        callback({}, {ApiErrorType::ServiceUnavailable, QStringLiteral("教务 API 服务不可用")});
        return;
    }
    m_api->fetchExamPlan([callback = std::move(callback)](const QList<ExamPlanItemDto> &items, const ApiError &error) {
        callback(items, error);
    });
}

void ZhjwApiQueryService::fetchSchemeScores(JsonCallback callback)
{
    // 保留 B 层原始 JSON 和 ApiError，让成绩 ViewModel 统一做领域解析与错误展示。
    if (!m_api) {
        callback({}, {ApiErrorType::ServiceUnavailable, QStringLiteral("教务 API 服务不可用")});
        return;
    }
    m_api->fetchSchemeScores([callback = std::move(callback)](const QJsonObject &root, const ApiError &error) {
        callback(root, error);
    });
}

void ZhjwApiQueryService::fetchPassingScores(JsonCallback callback)
{
    if (!m_api) {
        callback({}, {ApiErrorType::ServiceUnavailable, QStringLiteral("教务 API 服务不可用")});
        return;
    }
    m_api->fetchPassingScores([callback = std::move(callback)](const QJsonObject &root, const ApiError &error) {
        callback(root, error);
    });
}
