#include "src/viewmodels/ClassroomViewModel.h"

#include "src/models/ClassroomModels.h"
#include "src/services/zhjw/ZhjwQueryService.h"

#include <QTime>
#include <algorithm>

ClassroomViewModel::ClassroomViewModel(ZhjwQueryService *service, QObject *parent)
    : QObject(parent)
    , m_service(service)
{
}

QueryState ClassroomViewModel::state() const { return m_state; }
bool ClassroomViewModel::loading() const
{
    return m_state == QueryState::Loading || m_state == QueryState::Refreshing;
}
QString ClassroomViewModel::errorMessage() const { return m_errorMessage; }
QString ClassroomViewModel::viewMode() const { return m_viewMode; }

QVariantList ClassroomViewModel::campuses() const
{
    QVariantList values;
    for (const ClassroomCampusDto &campus : m_index.campuses) {
        values.append(QVariantMap{{QStringLiteral("name"), campus.campusName},
                                  {QStringLiteral("number"), campus.campusNumber}});
    }
    return values;
}

QVariantList ClassroomViewModel::buildings() const
{
    QVariantList values;
    for (const ClassroomBuildingDto &building : m_visibleBuildings) {
        values.append(QVariantMap{{QStringLiteral("name"), building.teachingBuildingName},
                                  {QStringLiteral("number"), building.teachingBuildingNumber}});
    }
    return values;
}

QVariantList ClassroomViewModel::rooms() const
{
    QVariantList values;
    for (const ClassroomInfoDto &room : m_result.classrooms) {
        const QVector<ClassroomPeriodStatus> statuses =
            classroomPeriodStatuses(m_result, room.classroomNumber);
        bool matches = true;
        if (m_filterPeriodStart > 0 && m_filterPeriodEnd >= m_filterPeriodStart) {
            for (int period = m_filterPeriodStart; period <= m_filterPeriodEnd; ++period) {
                if (statuses.at(period - 1) != ClassroomPeriodStatus::Free) {
                    matches = false;
                    break;
                }
            }
        }
        if (matches) {
            values.append(roomToVariant(room));
        }
    }
    return values;
}

QString ClassroomViewModel::selectedCampusName() const
{
    return m_selectedCampus >= 0 && m_selectedCampus < m_index.campuses.size()
        ? m_index.campuses.at(m_selectedCampus).campusName
        : QString();
}

QString ClassroomViewModel::selectedBuildingName() const
{
    return m_selectedBuilding >= 0 && m_selectedBuilding < m_visibleBuildings.size()
        ? m_visibleBuildings.at(m_selectedBuilding).teachingBuildingName
        : QString();
}

QString ClassroomViewModel::selectedDate() const { return m_selectedDate.toString(Qt::ISODate); }
bool ClassroomViewModel::selectedDateIsToday() const { return m_selectedDate == QDate::currentDate(); }
int ClassroomViewModel::teachingWeek() const { return m_result.teachingWeek; }
int ClassroomViewModel::filterPeriodStart() const { return m_filterPeriodStart; }
int ClassroomViewModel::filterPeriodEnd() const { return m_filterPeriodEnd; }
int ClassroomViewModel::currentPeriod() const
{
    return selectedDateIsToday()
        ? currentClassroomPeriod(selectedCampusName(), QTime::currentTime())
        : 0;
}

void ClassroomViewModel::load()
{
    if (loading()) {
        return;
    }
    if (!m_service || !m_service->loggedIn()) {
        setState(QueryState::LoginRequired);
        return;
    }
    loadIndex();
}

void ClassroomViewModel::refresh()
{
    if (loading()) {
        return;
    }
    if (!m_service || !m_service->loggedIn()) {
        finishWithError({ApiErrorType::Unauthenticated, QStringLiteral("请先登录教务系统")},
                        m_hasRoomResult);
        return;
    }
    if (m_viewMode == QStringLiteral("room") && m_selectedBuilding >= 0) {
        loadRooms();
    } else {
        loadIndex();
    }
}

void ClassroomViewModel::selectCampus(int index)
{
    if (index < 0 || index >= m_index.campuses.size()) {
        return;
    }
    cancelPendingRequest();
    m_selectedCampus = index;
    m_selectedBuilding = -1;
    m_visibleBuildings.clear();
    const QString campusNumber = m_index.campuses.at(index).campusNumber;
    for (const ClassroomBuildingDto &building : m_index.buildings) {
        if (building.campusNumber == campusNumber) {
            m_visibleBuildings.append(building);
        }
    }
    m_viewMode = QStringLiteral("building");
    m_hasRoomResult = false;
    m_result = {};
    emit selectionChanged();
    emit indexChanged();
    emit navigationChanged();
    emit roomsChanged();
    emit currentPeriodChanged();
}

void ClassroomViewModel::selectBuilding(int index)
{
    if (index < 0 || index >= m_visibleBuildings.size() || m_selectedCampus < 0) {
        return;
    }
    cancelPendingRequest();
    m_selectedBuilding = index;
    m_viewMode = QStringLiteral("room");
    m_hasRoomResult = false;
    m_result = {};
    emit selectionChanged();
    emit navigationChanged();
    emit roomsChanged();
    loadRooms();
}

void ClassroomViewModel::goBack()
{
    if (m_viewMode == QStringLiteral("room")) {
        cancelPendingRequest();
        m_viewMode = QStringLiteral("building");
        m_selectedBuilding = -1;
        m_result = {};
        m_hasRoomResult = false;
        emit selectionChanged();
        emit roomsChanged();
        emit navigationChanged();
        setState(m_index.campuses.isEmpty() ? QueryState::Empty : QueryState::Loaded);
        return;
    }
    if (m_viewMode == QStringLiteral("building")) {
        cancelPendingRequest();
        m_viewMode = QStringLiteral("campus");
        m_selectedCampus = -1;
        m_visibleBuildings.clear();
        emit selectionChanged();
        emit indexChanged();
        emit navigationChanged();
        emit currentPeriodChanged();
    }
}

void ClassroomViewModel::setSelectedDate(const QString &date)
{
    const QDate parsed = QDate::fromString(date, Qt::ISODate);
    const QDate today = QDate::currentDate();
    if (!parsed.isValid() || parsed < today.addDays(-7) || parsed > today.addDays(30)
        || parsed == m_selectedDate) {
        return;
    }
    m_selectedDate = parsed;
    emit dateChanged();
    emit currentPeriodChanged();
    if (m_viewMode == QStringLiteral("room") && m_selectedBuilding >= 0) {
        cancelPendingRequest();
        loadRooms();
    }
}

void ClassroomViewModel::setPeriodFilter(int start, int end)
{
    if (start < 1 || end < start || end > 12) {
        clearPeriodFilter();
        return;
    }
    if (m_filterPeriodStart == start && m_filterPeriodEnd == end) {
        return;
    }
    m_filterPeriodStart = start;
    m_filterPeriodEnd = end;
    emit filterChanged();
    emit roomsChanged();
}

void ClassroomViewModel::filterCurrentPeriod()
{
    const int period = currentPeriod();
    if (period > 0) {
        setPeriodFilter(period, period);
    }
}

void ClassroomViewModel::clearPeriodFilter()
{
    if (m_filterPeriodStart == 0 && m_filterPeriodEnd == 0) {
        return;
    }
    m_filterPeriodStart = 0;
    m_filterPeriodEnd = 0;
    emit filterChanged();
    emit roomsChanged();
}

void ClassroomViewModel::loadIndex()
{
    if (!m_service || m_requestInFlight) {
        return;
    }
    m_requestInFlight = true;
    const quint64 requestGeneration = ++m_requestGeneration;
    setError(QString());
    setState(m_index.campuses.isEmpty() ? QueryState::Loading : QueryState::Refreshing);
    m_service->fetchClassroomIndex([this, requestGeneration](const ClassroomIndexDto &index,
                                                            const ApiError &error) {
        if (requestGeneration != m_requestGeneration) {
            return;
        }
        m_requestInFlight = false;
        if (hasError(error)) {
            finishWithError(error, !m_index.campuses.isEmpty());
            return;
        }
        m_index = index;
        setError(QString());
        emit indexChanged();
        setState(m_index.campuses.isEmpty() ? QueryState::Empty : QueryState::Loaded);
    });
}

void ClassroomViewModel::loadRooms()
{
    if (!m_service || m_requestInFlight || m_selectedCampus < 0 || m_selectedBuilding < 0) {
        return;
    }
    m_requestInFlight = true;
    const quint64 requestGeneration = ++m_requestGeneration;
    setError(QString());
    setState(m_hasRoomResult ? QueryState::Refreshing : QueryState::Loading);
    const QString campusNumber = m_index.campuses.at(m_selectedCampus).campusNumber;
    const QString buildingNumber =
        m_visibleBuildings.at(m_selectedBuilding).teachingBuildingNumber;
    m_service->fetchClassroomAvailability(
        campusNumber,
        buildingNumber,
        selectedDate(),
        [this, requestGeneration](const ClassroomQueryResultDto &result, const ApiError &error) {
            if (requestGeneration != m_requestGeneration) {
                return;
            }
            m_requestInFlight = false;
            if (hasError(error)) {
                finishWithError(error, m_hasRoomResult);
                return;
            }
            m_result = result;
            m_hasRoomResult = true;
            setError(QString());
            emit roomsChanged();
            setState(m_result.classrooms.isEmpty() ? QueryState::Empty : QueryState::Loaded);
        });
}

void ClassroomViewModel::cancelPendingRequest()
{
    if (!m_requestInFlight) {
        return;
    }
    ++m_requestGeneration;
    m_requestInFlight = false;
}

void ClassroomViewModel::setState(QueryState state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    emit stateChanged();
}

void ClassroomViewModel::setError(const QString &message)
{
    if (m_errorMessage == message) {
        return;
    }
    m_errorMessage = message;
    emit errorChanged();
}

void ClassroomViewModel::finishWithError(const ApiError &error, bool keepRooms)
{
    setError(error.message);
    if (keepRooms) {
        emit toastRequested(error.message);
        setState(m_result.classrooms.isEmpty() ? QueryState::Empty : QueryState::Loaded);
        return;
    }
    const bool loginRequired = error.type == ApiErrorType::Unauthenticated
        || error.type == ApiErrorType::SessionExpired;
    setState(loginRequired ? QueryState::LoginRequired : QueryState::Error);
}

bool ClassroomViewModel::hasError(const ApiError &error) const
{
    return error.type != ApiErrorType::Unknown;
}

QVariantMap ClassroomViewModel::roomToVariant(const ClassroomInfoDto &room) const
{
    QVariantList periods;
    const QVector<ClassroomPeriodStatus> statuses =
        classroomPeriodStatuses(m_result, room.classroomNumber);
    for (int index = 0; index < statuses.size(); ++index) {
        periods.append(QVariantMap{{QStringLiteral("period"), index + 1},
                                   {QStringLiteral("statusKey"), classroomStatusKey(statuses.at(index))},
                                   {QStringLiteral("statusText"), classroomStatusText(statuses.at(index))}});
    }
    return {{QStringLiteral("name"), room.classroomName},
            {QStringLiteral("number"), room.classroomNumber},
            {QStringLiteral("seats"), room.placeNum},
            {QStringLiteral("remark"), room.remark},
            {QStringLiteral("borrowable"), room.borrowable},
            {QStringLiteral("periods"), periods}};
}
