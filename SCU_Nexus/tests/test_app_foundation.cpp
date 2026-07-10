#include <QtTest>
#include "src/app/AppController.h"
#include "src/app/AppSettings.h"
#include "src/common/QueryState.h"

class AppFoundationTests final : public QObject
{
    Q_OBJECT
private slots:
    void startupInitializationRunsStepsAndBecomesReady()
    {
        QStringList callOrder;
        int cacheCalls = 0;
        int scheduleCalls = 0;
        AppController controller(
            nullptr,
            nullptr,
            [&callOrder, &cacheCalls]() {
                callOrder.append(QStringLiteral("cache"));
                ++cacheCalls;
                return StartupStepResult{};
            },
            [&callOrder, &scheduleCalls]() {
                callOrder.append(QStringLiteral("schedule"));
                ++scheduleCalls;
                return StartupStepResult{};
            });
        QSignalSpy finishedSpy(&controller, &AppController::startupFinished);

        controller.initialize();

        QVERIFY(controller.ready());
        QVERIFY(controller.startupError().isEmpty());
        QCOMPARE(cacheCalls, 1);
        QCOMPARE(scheduleCalls, 1);
        QCOMPARE(callOrder, QStringList({QStringLiteral("cache"), QStringLiteral("schedule")}));
        QCOMPARE(finishedSpy.count(), 1);
    }

    void startupInitializationAfterSuccessIsNoOp()
    {
        int cacheCalls = 0;
        int scheduleCalls = 0;
        AppController controller(
            nullptr,
            nullptr,
            [&cacheCalls]() {
                ++cacheCalls;
                return StartupStepResult{};
            },
            [&scheduleCalls]() {
                ++scheduleCalls;
                return StartupStepResult{};
            });
        QSignalSpy finishedSpy(&controller, &AppController::startupFinished);

        controller.initialize();
        controller.initialize();

        QVERIFY(controller.ready());
        QVERIFY(controller.startupError().isEmpty());
        QCOMPARE(cacheCalls, 1);
        QCOMPARE(scheduleCalls, 1);
        QCOMPARE(finishedSpy.count(), 1);
    }

    void failedCacheInitializationCanBeRetried()
    {
        int cacheCalls = 0;
        int scheduleCalls = 0;
        AppController controller(
            nullptr,
            nullptr,
            [&cacheCalls]() {
                ++cacheCalls;
                if (cacheCalls == 1) {
                    return StartupStepResult{false, QStringLiteral("磁盘不可写")};
                }
                return StartupStepResult{};
            },
            [&scheduleCalls]() {
                ++scheduleCalls;
                return StartupStepResult{};
            });
        QSignalSpy failedSpy(&controller, &AppController::startupFailed);
        QSignalSpy finishedSpy(&controller, &AppController::startupFinished);

        controller.initialize();

        QVERIFY(!controller.ready());
        QVERIFY(controller.startupError().contains(QStringLiteral("查询缓存")));
        QVERIFY(controller.startupError().contains(QStringLiteral("磁盘不可写")));
        QCOMPARE(cacheCalls, 1);
        QCOMPARE(scheduleCalls, 0);
        QCOMPARE(failedSpy.count(), 1);
        QCOMPARE(finishedSpy.count(), 0);

        controller.initialize();

        QVERIFY(controller.ready());
        QVERIFY(controller.startupError().isEmpty());
        QCOMPARE(cacheCalls, 2);
        QCOMPARE(scheduleCalls, 1);
        QCOMPARE(failedSpy.count(), 1);
        QCOMPARE(finishedSpy.count(), 1);
    }

    void queryStateHasStableRefreshingValue()
    {
        QCOMPARE(static_cast<int>(QueryState::Idle), 0);
        QCOMPARE(static_cast<int>(QueryState::Loading), 1);
        QCOMPARE(static_cast<int>(QueryState::Loaded), 2);
        QCOMPARE(static_cast<int>(QueryState::Refreshing), 3);
        QCOMPARE(static_cast<int>(QueryState::Empty), 4);
        QCOMPARE(static_cast<int>(QueryState::Error), 5);
        QCOMPARE(static_cast<int>(QueryState::LoginRequired), 6);
    }

    void windowGeometryUsesRequiredDefaults()
    {
        const QRect actual = AppSettings::sanitizedGeometry({}, QRect(0, 0, 1920, 1080));
        QCOMPARE(actual.size(), QSize(1100, 760));
        QVERIFY(QRect(0, 0, 1920, 1080).contains(actual));
    }

    void windowGeometryClampsSizeAndPosition()
    {
        QCOMPARE(AppSettings::sanitizedGeometry(QRect(-500, -500, 300, 200),
                                                 QRect(0, 0, 1024, 700)),
                 QRect(0, 0, 900, 620));
    }
};

QTEST_MAIN(AppFoundationTests)
#include "test_app_foundation.moc"
