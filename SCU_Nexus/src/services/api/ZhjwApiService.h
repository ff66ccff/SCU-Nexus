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

class ZhjwApiService : public QObject
{
    Q_OBJECT
public:
    using WeekCallback = std::function<void(int week, const ApiError& error)>;
    using SemestersCallback = std::function<void(const QList<SemesterDto>& semesters, const ApiError& error)>;
    using ScheduleCallback = std::function<void(const QJsonObject& schedule, const ApiError& error)>;
    using ExamPlanCallback = std::function<void(const QList<ExamPlanItemDto>& exams, const ApiError& error)>;
    using JsonCallback = std::function<void(const QJsonObject& json, const ApiError& error)>;

    explicit ZhjwApiService(QObject* parent = nullptr, ZhjwAuthService* authService = nullptr);

    static QString scuIdBase();
    static QString zhjwBase();
    static QString jwcCalendarUrl();
    static bool isSessionExpired(const QString& body, int statusCode);

    void fetchCurrentWeek(WeekCallback callback);
    void fetchSemesters(SemestersCallback callback);
    void fetchJwxtSchedule(const QString& planCode, ScheduleCallback callback);
    void fetchExamPlan(ExamPlanCallback callback);
    void fetchSchemeScores(JsonCallback callback);
    void fetchPassingScores(JsonCallback callback);

private:
    using ResponseCallback = std::function<void(const HttpResponse& response, const ApiError& error)>;

    void request(const QUrl& url,
                 const CookieHttpClient::Headers& headers,
                 ResponseCallback callback,
                 bool retried = false);
    void postForm(const QUrl& url,
                  const QByteArray& body,
                  const CookieHttpClient::Headers& headers,
                  ResponseCallback callback,
                  bool retried = false);
    void fetchScoreJson(const QString& indexPath,
                        const QString& callbackKind,
                        const QString& parseFailureMessage,
                        JsonCallback callback);
    static QJsonObject parseJsonObject(const QByteArray& body, const QString& context, ApiError* error);
    static ApiError makeError(ApiErrorType type, const QString& message, int statusCode = 0, const QString& debugBody = {});

    ZhjwAuthService* m_authService = nullptr;
    bool m_ownsAuthService = false;
};

#endif // ZHJWAPISERVICE_H
