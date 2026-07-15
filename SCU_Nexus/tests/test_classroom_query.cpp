#include "src/models/ClassroomModels.h"
#include "src/services/zhjw/ZhjwQueryService.h"
#include "src/viewmodels/ClassroomViewModel.h"

#include <QDate>
#include <QSignalSpy>
#include <QtTest>

#include <utility>

namespace {

ClassroomIndexDto sampleIndex()
{
    return {
        {ClassroomCampusDto{QStringLiteral("江安"), QStringLiteral("01")}},
        {ClassroomBuildingDto{QStringLiteral("01"),
                              QStringLiteral("101"),
                              QStringLiteral("一教")}}
    };
}

ClassroomQueryResultDto sampleResult()
{
    ClassroomQueryResultDto result;
    result.date = QDate::currentDate().toString(Qt::ISODate);
    result.teachingWeek = 20;
    result.classrooms = {
        ClassroomInfoDto{QStringLiteral("一教A101"),
                         QStringLiteral("01"),
                         QStringLiteral("01"),
                         QStringLiteral("01"),
                         QStringLiteral("A101"),
                         QStringLiteral("101"),
                         120,
                         QStringLiteral("多媒体"),
                         QStringLiteral("是")},
        ClassroomInfoDto{QStringLiteral("一教B201"),
                         QStringLiteral("01"),
                         QStringLiteral("01"),
                         QStringLiteral("01"),
                         QStringLiteral("B201"),
                         QStringLiteral("101"),
                         80,
                         QString(),
                         QStringLiteral("否")},
    };
    result.timeSlots = {
        ClassroomTimeSlotDto{QStringLiteral("01"),
                             QStringLiteral("101"),
                             QStringLiteral("A101"),
                             3,
                             2,
                             2,
                             QStringLiteral("busy"),
                             QStringLiteral("06")},
    };
    return result;
}

class FakeClassroomQueryService final : public ZhjwQueryService
{
public:
    bool loggedInValue = true;
    int indexCalls = 0;
    int roomCalls = 0;
    bool deferIndex = false;
    ClassroomIndexDto index = sampleIndex();
    ClassroomQueryResultDto result = sampleResult();
    ApiError nextIndexError;
    ApiError nextRoomError;
    QString lastCampus;
    QString lastBuilding;
    QString lastDate;
    ClassroomIndexCallback pendingIndex;

    bool loggedIn() const override { return loggedInValue; }

    void setLoggedIn(bool value) override
    {
        if (loggedInValue == value) {
            return;
        }
        loggedInValue = value;
        emit loggedInChanged();
    }

    void fetchExamPlan(ExamPlanCallback callback) override { callback({}, {}); }
    void fetchSchemeScores(JsonCallback callback) override { callback({}, {}); }
    void fetchPassingScores(JsonCallback callback) override { callback({}, {}); }

    void fetchClassroomIndex(ClassroomIndexCallback callback) override
    {
        ++indexCalls;
        if (deferIndex) {
            pendingIndex = std::move(callback);
            return;
        }
        callback(index, std::exchange(nextIndexError, ApiError{}));
    }

    void fetchClassroomAvailability(const QString& campusNumber,
                                    const QString& buildingNumber,
                                    const QString& searchDate,
                                    ClassroomAvailabilityCallback callback) override
    {
        ++roomCalls;
        lastCampus = campusNumber;
        lastBuilding = buildingNumber;
        lastDate = searchDate;
        callback(result, std::exchange(nextRoomError, ApiError{}));
    }

    void completeIndex()
    {
        QVERIFY(pendingIndex);
        auto callback = std::move(pendingIndex);
        callback(index, std::exchange(nextIndexError, ApiError{}));
    }
};

}

class ClassroomQueryTests final : public QObject
{
    Q_OBJECT

private slots:
    void expandsContinuousPeriodsAndMapsStatuses();
    void computesCurrentPeriodForCampusTimetable();
    void loadsCampusThenBuildingThenRooms();
    void filtersRoomsAcrossInclusivePeriodRange();
    void changingDateReloadsSelectedBuilding();
    void mapsLoginAndNetworkErrorsToQueryState();
    void ignoresDuplicateRefreshWhileLoading();
    void refreshFailureKeepsCurrentRoomsAndShowsToast();
};

void ClassroomQueryTests::expandsContinuousPeriodsAndMapsStatuses()
{
    ClassroomQueryResultDto result;
    result.timeSlots = {
        ClassroomTimeSlotDto{QStringLiteral("01"),
                             QStringLiteral("101"),
                             QStringLiteral("A101"),
                             3,
                             2,
                             2,
                             QStringLiteral("busy"),
                             QStringLiteral("06")},
        ClassroomTimeSlotDto{QStringLiteral("01"),
                             QStringLiteral("101"),
                             QStringLiteral("A101"),
                             3,
                             5,
                             1,
                             QStringLiteral("busy"),
                             QStringLiteral("07")},
        ClassroomTimeSlotDto{QStringLiteral("01"),
                             QStringLiteral("101"),
                             QStringLiteral("A101"),
                             3,
                             6,
                             1,
                             QStringLiteral("busy"),
                             QStringLiteral("14")},
        ClassroomTimeSlotDto{QStringLiteral("01"),
                             QStringLiteral("101"),
                             QStringLiteral("A101"),
                             3,
                             7,
                             1,
                             QStringLiteral("busy"),
                             QStringLiteral("room")},
    };

    const QVector<ClassroomPeriodStatus> statuses =
        classroomPeriodStatuses(result, QStringLiteral("A101"));

    QCOMPARE(statuses.size(), 12);
    QCOMPARE(statuses.at(0), ClassroomPeriodStatus::Free);
    QCOMPARE(statuses.at(1), ClassroomPeriodStatus::InClass);
    QCOMPARE(statuses.at(2), ClassroomPeriodStatus::InClass);
    QCOMPARE(statuses.at(4), ClassroomPeriodStatus::Exam);
    QCOMPARE(statuses.at(5), ClassroomPeriodStatus::Experiment);
    QCOMPARE(statuses.at(6), ClassroomPeriodStatus::Borrowed);
    QCOMPARE(classroomStatusKey(statuses.at(4)), QStringLiteral("exam"));
    QCOMPARE(classroomStatusText(statuses.at(6)), QStringLiteral("借用"));
}

void ClassroomQueryTests::computesCurrentPeriodForCampusTimetable()
{
    QCOMPARE(currentClassroomPeriod(QStringLiteral("江安"), QTime(8, 0)), 1);
    QCOMPARE(currentClassroomPeriod(QStringLiteral("江安"), QTime(9, 5)), 2);
    QCOMPARE(currentClassroomPeriod(QStringLiteral("望江"), QTime(13, 50)), 5);
    QCOMPARE(currentClassroomPeriod(QStringLiteral("未知校区"), QTime(8, 0)), 0);
    QCOMPARE(currentClassroomPeriod(QStringLiteral("江安"), QTime(23, 30)), 0);
}

void ClassroomQueryTests::loadsCampusThenBuildingThenRooms()
{
    FakeClassroomQueryService service;
    ClassroomViewModel model(&service);

    model.load();
    QCOMPARE(service.indexCalls, 1);
    QCOMPARE(model.state(), QueryState::Loaded);
    QCOMPARE(model.viewMode(), QStringLiteral("campus"));
    QCOMPARE(model.campuses().size(), 1);

    model.selectCampus(0);
    QCOMPARE(model.viewMode(), QStringLiteral("building"));
    QCOMPARE(model.selectedCampusName(), QStringLiteral("江安"));
    QCOMPARE(model.buildings().size(), 1);

    model.selectBuilding(0);
    QCOMPARE(service.roomCalls, 1);
    QCOMPARE(service.lastCampus, QStringLiteral("01"));
    QCOMPARE(service.lastBuilding, QStringLiteral("101"));
    QCOMPARE(model.viewMode(), QStringLiteral("room"));
    QCOMPARE(model.selectedBuildingName(), QStringLiteral("一教"));
    QCOMPARE(model.rooms().size(), 2);
    QCOMPARE(model.teachingWeek(), 20);
}

void ClassroomQueryTests::filtersRoomsAcrossInclusivePeriodRange()
{
    FakeClassroomQueryService service;
    ClassroomViewModel model(&service);
    model.load();
    model.selectCampus(0);
    model.selectBuilding(0);

    model.setPeriodFilter(2, 3);

    const QVariantList rooms = model.rooms();
    QCOMPARE(rooms.size(), 1);
    QCOMPARE(rooms.first().toMap().value(QStringLiteral("number")).toString(),
             QStringLiteral("B201"));
    const QVariantList periods = rooms.first().toMap()
                                     .value(QStringLiteral("periods"))
                                     .toList();
    QCOMPARE(periods.size(), 12);
}

void ClassroomQueryTests::changingDateReloadsSelectedBuilding()
{
    FakeClassroomQueryService service;
    ClassroomViewModel model(&service);
    model.load();
    model.selectCampus(0);
    model.selectBuilding(0);
    QCOMPARE(service.roomCalls, 1);

    const QString tomorrow = QDate::currentDate().addDays(1).toString(Qt::ISODate);
    model.setSelectedDate(tomorrow);

    QCOMPARE(service.roomCalls, 2);
    QCOMPARE(service.lastDate, tomorrow);
    QCOMPARE(model.selectedDate(), tomorrow);
}

void ClassroomQueryTests::mapsLoginAndNetworkErrorsToQueryState()
{
    FakeClassroomQueryService loggedOut;
    loggedOut.loggedInValue = false;
    ClassroomViewModel loggedOutModel(&loggedOut);
    loggedOutModel.load();
    QCOMPARE(loggedOutModel.state(), QueryState::LoginRequired);
    QCOMPARE(loggedOut.indexCalls, 0);

    FakeClassroomQueryService failing;
    failing.nextIndexError = {ApiErrorType::Network, QStringLiteral("网络不可用")};
    ClassroomViewModel failingModel(&failing);
    failingModel.load();
    QCOMPARE(failingModel.state(), QueryState::Error);
    QCOMPARE(failingModel.errorMessage(), QStringLiteral("网络不可用"));
}

void ClassroomQueryTests::ignoresDuplicateRefreshWhileLoading()
{
    FakeClassroomQueryService service;
    service.deferIndex = true;
    ClassroomViewModel model(&service);

    model.load();
    model.refresh();

    QCOMPARE(service.indexCalls, 1);
    QCOMPARE(model.state(), QueryState::Loading);
    service.completeIndex();
    QCOMPARE(model.state(), QueryState::Loaded);
}

void ClassroomQueryTests::refreshFailureKeepsCurrentRoomsAndShowsToast()
{
    FakeClassroomQueryService service;
    ClassroomViewModel model(&service);
    model.load();
    model.selectCampus(0);
    model.selectBuilding(0);
    QCOMPARE(model.rooms().size(), 2);
    QSignalSpy toastSpy(&model, &ClassroomViewModel::toastRequested);

    service.nextRoomError = {ApiErrorType::Network, QStringLiteral("刷新失败")};
    model.refresh();

    QCOMPARE(model.state(), QueryState::Loaded);
    QCOMPARE(model.rooms().size(), 2);
    QCOMPARE(toastSpy.size(), 1);
    QCOMPARE(toastSpy.first().first().toString(), QStringLiteral("刷新失败"));
}

QTEST_APPLESS_MAIN(ClassroomQueryTests)

#include "test_classroom_query.moc"
