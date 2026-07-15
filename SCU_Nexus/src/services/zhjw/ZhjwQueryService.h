#pragma once

#include "src/core/network/NetworkError.h"
#include "src/services/api/ApiDtos.h"

#include <QJsonObject>
#include <QObject>
#include <functional>

class ZhjwApiService;

// 考表/成绩 ViewModel 面向的窄接口，便于用假服务测试页面状态和缓存逻辑。
// 真实实现 ZhjwApiQueryService 仅转发 B 层 API；loggedIn 由应用装配层同步。
class ZhjwQueryService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY loggedInChanged)

public:
    using ExamPlanCallback = std::function<void(const QList<ExamPlanItemDto> &items, const ApiError &error)>;
    using JsonCallback = std::function<void(const QJsonObject &root, const ApiError &error)>;
    using ClassroomIndexCallback = std::function<void(const ClassroomIndexDto &index, const ApiError &error)>;
    using ClassroomAvailabilityCallback = std::function<void(const ClassroomQueryResultDto &result, const ApiError &error)>;

    explicit ZhjwQueryService(QObject *parent = nullptr);
    ~ZhjwQueryService() override = default;

    virtual bool loggedIn() const = 0;
    virtual void setLoggedIn(bool loggedIn) = 0;
    virtual void fetchExamPlan(ExamPlanCallback callback) = 0;
    virtual void fetchSchemeScores(JsonCallback callback) = 0;
    virtual void fetchPassingScores(JsonCallback callback) = 0;
    virtual void fetchClassroomIndex(ClassroomIndexCallback callback);
    virtual void fetchClassroomAvailability(const QString &campusNumber,
                                            const QString &buildingNumber,
                                            const QString &searchDate,
                                            ClassroomAvailabilityCallback callback);

signals:
    void loggedInChanged();
};

class ZhjwApiQueryService : public ZhjwQueryService
{
public:
    explicit ZhjwApiQueryService(QObject *parent = nullptr, ZhjwApiService *api = nullptr);

    bool loggedIn() const override;
    void setLoggedIn(bool loggedIn) override;
    void fetchExamPlan(ExamPlanCallback callback) override;
    void fetchSchemeScores(JsonCallback callback) override;
    void fetchPassingScores(JsonCallback callback) override;
    void fetchClassroomIndex(ClassroomIndexCallback callback) override;
    void fetchClassroomAvailability(const QString &campusNumber,
                                    const QString &buildingNumber,
                                    const QString &searchDate,
                                    ClassroomAvailabilityCallback callback) override;

private:
    ZhjwApiService *m_api = nullptr;
    bool m_ownsApi = false;
    bool m_loggedIn = false;
};
