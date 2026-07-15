#include "src/services/zhjw/ZhjwQueryService.h"

ZhjwQueryService::ZhjwQueryService(QObject *parent)
    : QObject(parent)
{
}

void ZhjwQueryService::fetchClassroomIndex(ClassroomIndexCallback callback)
{
    callback({}, {ApiErrorType::ServiceUnavailable, QStringLiteral("教室查询服务不可用")});
}

void ZhjwQueryService::fetchClassroomAvailability(const QString &campusNumber,
                                                  const QString &buildingNumber,
                                                  const QString &searchDate,
                                                  ClassroomAvailabilityCallback callback)
{
    Q_UNUSED(campusNumber)
    Q_UNUSED(buildingNumber)
    Q_UNUSED(searchDate)
    callback({}, {ApiErrorType::ServiceUnavailable, QStringLiteral("教室查询服务不可用")});
}
