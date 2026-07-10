#include <QtTest>
#include "src/app/AppSettings.h"
#include "src/common/QueryState.h"

class AppFoundationTests final : public QObject
{
    Q_OBJECT
private slots:
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
