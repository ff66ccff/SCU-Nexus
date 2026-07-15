#ifndef ZHJWAPISERVICE_H
#define ZHJWAPISERVICE_H

#include "src/core/network/CookieHttpClient.h"
#include <QObject>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <functional>

#include "src/services/api/ApiDtos.h"

class ZhjwAuthService;

// 教务远端 API 门面。
//
// 这里负责认证、请求、会话失效重试和“远端格式”解析；不负责课表入库、考表排序、
// 成绩模型/GPA 统计。课表和成绩接口因此返回原始 QJsonObject，避免跨模块重复建模。
class ZhjwApiService : public QObject
{
    Q_OBJECT
public:
    using WeekCallback = std::function<void(int week, const ApiError& error)>;
    using SemestersCallback = std::function<void(const QList<SemesterDto>& semesters, const ApiError& error)>;
    using ScheduleCallback = std::function<void(const QJsonObject& schedule, const ApiError& error)>;
    using ExamPlanCallback = std::function<void(const QList<ExamPlanItemDto>& exams, const ApiError& error)>;
    using JsonCallback = std::function<void(const QJsonObject& json, const ApiError& error)>;
    using ClassroomIndexCallback = std::function<void(const ClassroomIndexDto& index, const ApiError& error)>;
    using ClassroomAvailabilityCallback = std::function<void(const ClassroomQueryResultDto& result, const ApiError& error)>;

    explicit ZhjwApiService(QObject* parent = nullptr, ZhjwAuthService* authService = nullptr);

    static QString scuIdBase();
    static QString zhjwBase();
    static QString jwcCalendarUrl();
    static bool isSessionExpired(const QString& body, int statusCode);

    void fetchCurrentWeek(WeekCallback callback);
    void fetchSemesters(SemestersCallback callback);
    void fetchJwxtSchedule(const QString& planCode, ScheduleCallback callback);
    void fetchClassroomIndex(ClassroomIndexCallback callback);
    void fetchClassroomAvailability(const QString& campusNumber,
                                    const QString& buildingNumber,
                                    const QString& searchDate,
                                    ClassroomAvailabilityCallback callback);
    void fetchExamPlan(ExamPlanCallback callback);
    void fetchSchemeScores(JsonCallback callback);
    void fetchPassingScores(JsonCallback callback);

private:
    using ResponseCallback = std::function<void(const HttpResponse& response, const ApiError& error)>;

    // GET/表单 POST 的公共包装：获取 SSO client，识别失效响应，并且只重试一次。
    void request(const QUrl& url,
                 const CookieHttpClient::Headers& headers,
                 ResponseCallback callback,
                 bool retried = false);
    void postForm(const QUrl& url,
                  const QByteArray& body,
                  const CookieHttpClient::Headers& headers,
                  ResponseCallback callback,
                  bool retried = false);
    // 成绩接口是两步请求：入口 HTML 给出动态 callback path，callback 才返回 JSON。
    void fetchScoreJson(const QString& indexPath,
                        const QString& callbackKind,
                        const QString& parseFailureMessage,
                        JsonCallback callback,
                        bool retried = false);
    static QJsonObject parseJsonObject(const QByteArray& body, const QString& context, ApiError* error);
    static ApiError makeError(ApiErrorType type, const QString& message, int statusCode = 0, const QString& debugBody = {});

    ZhjwAuthService* m_authService = nullptr;
    bool m_ownsAuthService = false;
};

#endif // ZHJWAPISERVICE_H
