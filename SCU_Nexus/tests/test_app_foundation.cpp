#include <QtTest>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QTemporaryDir>
#include "src/app/AppController.h"
#include "src/app/AppSettings.h"
#include "src/common/QueryState.h"
#include "src/repositories/QueryCacheRepository.h"
#include "src/viewmodels/QueryCacheViewModel.h"

namespace {

QString currentIniUserPath()
{
    const QString probeOrganization = QStringLiteral("SCUNexusSettingsPathProbe");
    const QSettings probe(QSettings::IniFormat,
                          QSettings::UserScope,
                          probeOrganization,
                          QStringLiteral("PathProbe"));
    QString path = QDir::fromNativeSeparators(QFileInfo(probe.fileName()).absolutePath());
    const QString organizationSuffix = QStringLiteral("/") + probeOrganization;
    if (path.endsWith(organizationSuffix))
        path.chop(organizationSuffix.size());
    return path;
}

class TemporarySettingsScope final
{
public:
    explicit TemporarySettingsScope(const QString &path)
        : m_previousFormat(QSettings::defaultFormat())
        , m_previousOrganization(QCoreApplication::organizationName())
        , m_previousApplication(QCoreApplication::applicationName())
        , m_previousIniUserPath(currentIniUserPath())
    {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, path);
        QCoreApplication::setOrganizationName(QStringLiteral("SCUNexusTests"));
        QCoreApplication::setApplicationName(QStringLiteral("AppSettingsPersistence"));
        QSettings().clear();
    }

    ~TemporarySettingsScope()
    {
        QSettings().clear();
        QCoreApplication::setOrganizationName(m_previousOrganization);
        QCoreApplication::setApplicationName(m_previousApplication);
        QSettings::setPath(QSettings::IniFormat,
                           QSettings::UserScope,
                           m_previousIniUserPath);
        QSettings::setDefaultFormat(m_previousFormat);
    }

private:
    QSettings::Format m_previousFormat;
    QString m_previousOrganization;
    QString m_previousApplication;
    QString m_previousIniUserPath;
};

} // namespace

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

    void qwenApiKeyPersistsNormalizedChanges()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        TemporarySettingsScope settingsScope(directory.path());

        AppSettings first;
        QSignalSpy changed(&first, &AppSettings::qwenApiKeyChanged);

        QVERIFY(first.saveQwenApiKey(QStringLiteral("  sk-test  ")));
        QCOMPARE(first.qwenApiKey(), QStringLiteral("sk-test"));
        QVERIFY(first.hasQwenApiKey());
        QCOMPARE(changed.count(), 1);

        QVERIFY(first.saveQwenApiKey(QStringLiteral("sk-test")));
        QCOMPARE(changed.count(), 1);

        AppSettings restored;
        QCOMPARE(restored.qwenApiKey(), QStringLiteral("sk-test"));
        QVERIFY(restored.hasQwenApiKey());

        QSignalSpy restoredChanged(&restored, &AppSettings::qwenApiKeyChanged);
        QVERIFY(restored.saveQwenApiKey(QStringLiteral("   ")));
        QVERIFY(restored.qwenApiKey().isEmpty());
        QVERIFY(!restored.hasQwenApiKey());
        QCOMPARE(restoredChanged.count(), 1);

        QVERIFY(restored.clearQwenApiKey());
        QCOMPARE(restoredChanged.count(), 1);
    }

    void qwenApiKeyClearRemovesPersistedValue()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        TemporarySettingsScope settingsScope(directory.path());

        AppSettings first;
        QVERIFY(first.saveQwenApiKey(QStringLiteral("sk-clear")));

        AppSettings restored;
        QSignalSpy changed(&restored, &AppSettings::qwenApiKeyChanged);
        QVERIFY(restored.clearQwenApiKey());
        QVERIFY(!restored.hasQwenApiKey());
        QCOMPARE(changed.count(), 1);

        AppSettings cleared;
        QVERIFY(cleared.qwenApiKey().isEmpty());
        QVERIFY(!cleared.hasQwenApiKey());
    }

    void queryCacheClearRemovesStoredEntries()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());

        QueryCacheRepository repository(
            directory.filePath(QStringLiteral("query-cache.sqlite")));
        QVERIFY2(repository.open(), qPrintable(repository.lastError()));
        QVERIFY2(repository.put(QStringLiteral("exam-plan"),
                                QStringLiteral(R"({"items":[]})")),
                 qPrintable(repository.lastError()));

        QueryCacheEntry entry;
        QVERIFY(repository.get(QStringLiteral("exam-plan"), &entry));

        QueryCacheViewModel viewModel(&repository);
        QSignalSpy clearedSpy(&viewModel, &QueryCacheViewModel::cacheCleared);
        QSignalSpy errorSpy(&viewModel, &QueryCacheViewModel::errorMessageChanged);

        QVERIFY(viewModel.clearAll());
        QVERIFY(!repository.get(QStringLiteral("exam-plan"), &entry));
        QVERIFY(viewModel.errorMessage().isEmpty());
        QCOMPARE(clearedSpy.count(), 1);
        QCOMPARE(errorSpy.count(), 0);
    }

    void queryCacheClearReportsUninitializedRepository()
    {
        QueryCacheViewModel viewModel(nullptr);
        QSignalSpy clearedSpy(&viewModel, &QueryCacheViewModel::cacheCleared);
        QSignalSpy errorSpy(&viewModel, &QueryCacheViewModel::errorMessageChanged);

        QVERIFY(!viewModel.clearAll());
        QCOMPARE(viewModel.errorMessage(), QStringLiteral("缓存仓库未初始化"));
        QCOMPARE(clearedSpy.count(), 0);
        QCOMPARE(errorSpy.count(), 1);
    }
};

QTEST_MAIN(AppFoundationTests)
#include "test_app_foundation.moc"
