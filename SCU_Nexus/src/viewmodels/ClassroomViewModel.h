#pragma once

#include "src/common/QueryState.h"
#include "src/core/network/NetworkError.h"
#include "src/services/api/ApiDtos.h"

#include <QDate>
#include <QObject>
#include <QVariantList>

class ZhjwQueryService;

class ClassroomViewModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QueryStateNamespace::QueryState state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)
    Q_PROPERTY(QString viewMode READ viewMode NOTIFY navigationChanged)
    Q_PROPERTY(QVariantList campuses READ campuses NOTIFY indexChanged)
    Q_PROPERTY(QVariantList buildings READ buildings NOTIFY indexChanged)
    Q_PROPERTY(QVariantList rooms READ rooms NOTIFY roomsChanged)
    Q_PROPERTY(QString selectedCampusName READ selectedCampusName NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedBuildingName READ selectedBuildingName NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedDate READ selectedDate NOTIFY dateChanged)
    Q_PROPERTY(bool selectedDateIsToday READ selectedDateIsToday NOTIFY dateChanged)
    Q_PROPERTY(int teachingWeek READ teachingWeek NOTIFY roomsChanged)
    Q_PROPERTY(int filterPeriodStart READ filterPeriodStart NOTIFY filterChanged)
    Q_PROPERTY(int filterPeriodEnd READ filterPeriodEnd NOTIFY filterChanged)
    Q_PROPERTY(int currentPeriod READ currentPeriod NOTIFY currentPeriodChanged)

public:
    explicit ClassroomViewModel(ZhjwQueryService *service, QObject *parent = nullptr);

    QueryState state() const;
    bool loading() const;
    QString errorMessage() const;
    QString viewMode() const;
    QVariantList campuses() const;
    QVariantList buildings() const;
    QVariantList rooms() const;
    QString selectedCampusName() const;
    QString selectedBuildingName() const;
    QString selectedDate() const;
    bool selectedDateIsToday() const;
    int teachingWeek() const;
    int filterPeriodStart() const;
    int filterPeriodEnd() const;
    int currentPeriod() const;

    Q_INVOKABLE void load();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void selectCampus(int index);
    Q_INVOKABLE void selectBuilding(int index);
    Q_INVOKABLE void goBack();
    Q_INVOKABLE void setSelectedDate(const QString &date);
    Q_INVOKABLE void setPeriodFilter(int start, int end);
    Q_INVOKABLE void filterCurrentPeriod();
    Q_INVOKABLE void clearPeriodFilter();

signals:
    void stateChanged();
    void errorChanged();
    void indexChanged();
    void navigationChanged();
    void selectionChanged();
    void roomsChanged();
    void dateChanged();
    void filterChanged();
    void currentPeriodChanged();
    void toastRequested(const QString &message);

private:
    void loadIndex();
    void loadRooms();
    void cancelPendingRequest();
    void setState(QueryState state);
    void setError(const QString &message);
    void finishWithError(const ApiError &error, bool keepRooms);
    bool hasError(const ApiError &error) const;
    QVariantMap roomToVariant(const ClassroomInfoDto &room) const;

    ZhjwQueryService *m_service = nullptr;
    QueryState m_state = QueryState::Idle;
    QString m_errorMessage;
    QString m_viewMode = QStringLiteral("campus");
    ClassroomIndexDto m_index;
    ClassroomQueryResultDto m_result;
    QList<ClassroomBuildingDto> m_visibleBuildings;
    int m_selectedCampus = -1;
    int m_selectedBuilding = -1;
    QDate m_selectedDate = QDate::currentDate();
    int m_filterPeriodStart = 0;
    int m_filterPeriodEnd = 0;
    bool m_requestInFlight = false;
    quint64 m_requestGeneration = 0;
    bool m_hasRoomResult = false;
};
