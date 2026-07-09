#pragma once

#include "src/core/network/NetworkError.h"
#include "src/services/api/ApiDtos.h"

#include <QJsonObject>
#include <QObject>
#include <functional>

class ZhjwApiService;

class ZhjwQueryService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY loggedInChanged)

public:
    using ExamPlanCallback = std::function<void(const QList<ExamPlanItemDto> &items, const ApiError &error)>;
    using JsonCallback = std::function<void(const QJsonObject &root, const ApiError &error)>;

    explicit ZhjwQueryService(QObject *parent = nullptr);
    ~ZhjwQueryService() override = default;

    virtual bool loggedIn() const = 0;
    virtual void setLoggedIn(bool loggedIn) = 0;
    virtual void fetchExamPlan(ExamPlanCallback callback) = 0;
    virtual void fetchSchemeScores(JsonCallback callback) = 0;
    virtual void fetchPassingScores(JsonCallback callback) = 0;

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

private:
    ZhjwApiService *m_api = nullptr;
    bool m_ownsApi = false;
    bool m_loggedIn = false;
};
