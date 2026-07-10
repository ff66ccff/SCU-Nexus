# Core Shell and Authentication Compliance Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 补齐应用启动、窗口设置、统一查询状态、缓存清理以及认证/网络隐私边界，为后续课表和查询模块提供稳定接口。

**Architecture:** 保持 QML → ViewModel/AppController → Service/Repository 分层。`AppController` 统一初始化两个仓储，`AppSettings` 独立持久化窗口几何，`QueryState::Refreshing` 表示保留缓存的后台刷新；认证模块只输出分类错误和脱敏摘要。

**Tech Stack:** C++20、Qt 6.11 Core/Gui/QML/Quick/Network/Sql/Test、CMake 3.30、PowerShell、MinGW 13.1。

## Global Constraints

- 正式需求优先级为 V4/V5 → 五份团队文档 → `Bugaoshan-main` 业务语义 → 当前 Qt 架构。
- 只实现 REQ-001～REQ-012；REQ-013 仅验证扩展边界，不实现 P2。
- 默认窗口 1100×760，最小窗口 900×620。
- QML 不得包含业务 URL、SQL、Cookie、密码或 SM2 逻辑。
- 不保存明文密码；日志不得包含完整学号、用户名、密码、token、Cookie、验证码、成绩或响应正文。
- 缓存刷新失败必须保留旧数据和旧时间戳。
- 不修改或提交由行尾差异造成的既有工作树改动。

---

## File Map

- `SCU_Nexus/src/common/QueryState.h`: 增加共享 `Refreshing` 枚举值。
- `SCU_Nexus/qml/components/query/QueryStatePane.qml`: 使用 `queryState`，避免覆盖 `QQuickItem::state`。
- `SCU_Nexus/src/app/AppSettings.{h,cpp}`: 保存、恢复并钳制窗口几何。
- `SCU_Nexus/src/app/AppController.{h,cpp}`: 初始化缓存和课表仓储，暴露启动错误。
- `SCU_Nexus/src/viewmodels/QueryCacheViewModel.{h,cpp}`: 向设置页提供真实清理结果和错误。
- `SCU_Nexus/main.cpp`: 组装以上依赖并暴露 QML 上下文。
- `SCU_Nexus/src/core/network/NetworkError.h`: 补齐正式错误类别。
- `SCU_Nexus/src/core/network/CookieJar.{h,cpp}`、`CookieHttpClient.{h,cpp}`: 调试摘要不再暴露 Cookie 值。
- `SCU_Nexus/src/core/logging/AuthLogger.cpp`、`SCU_Nexus/src/services/auth/ScuAuthService.cpp`: 去除身份信息并扩展脱敏。
- `SCU_Nexus/src/services/api/ZhjwParsers.{h,cpp}`、`ZhjwApiService.cpp`: 区分考试空结果与结构解析失败。
- `SCU_Nexus/tests/test_app_foundation.cpp`: 应用设置、启动和共享状态测试。
- `SCU_Nexus/tests/test_person_b_foundation.cpp`: 安全、Cookie 和解析边界测试。
- `SCU_Nexus/CMakeLists.txt`: 注册应用基础测试并清理硬编码运行时假设。

### Task 1: Add the shared Refreshing state and remove the QML property collision

**Files:**
- Modify: `SCU_Nexus/src/common/QueryState.h:10-17`
- Modify: `SCU_Nexus/qml/components/query/QueryStatePane.qml:4-44`
- Modify: `SCU_Nexus/qml/pages/calendar/AcademicCalendarPage.qml`
- Modify: `SCU_Nexus/qml/pages/exam/ExamPlanPage.qml`
- Modify: `SCU_Nexus/qml/pages/grades/SchemeScoresTab.qml`
- Modify: `SCU_Nexus/qml/pages/grades/PassingScoresTab.qml`
- Modify: `SCU_Nexus/qml/pages/grades/CustomStatsTab.qml`
- Test: `SCU_Nexus/tests/test_app_foundation.cpp`

**Interfaces:**
- Consumes: existing integer QML state bindings.
- Produces: `QueryState::Refreshing == 3`; QML property `queryState`; pane remains hidden for Loaded and Refreshing.

- [ ] **Step 1: Create the failing shared-state test**

Create `SCU_Nexus/tests/test_app_foundation.cpp` with:

```cpp
#include <QtTest>
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
};

QTEST_MAIN(AppFoundationTests)
#include "test_app_foundation.moc"
```

- [ ] **Step 2: Register and run the test to verify failure**

Append an `app_foundation_tests` target in `SCU_Nexus/CMakeLists.txt` linking `Qt6::Core`, `Qt6::Gui`, `Qt6::Sql`, and `Qt6::Test`, initially compiling `tests/test_app_foundation.cpp` and `src/common/QueryState.{h,cpp}`. Register it with `QT_QPA_PLATFORM=offscreen` so its `QGuiApplication` is deterministic in CI.

Run:

```powershell
D:\QT\Tools\CMake_64\bin\cmake.exe --build D:\SCU-Nexus\SCU_Nexus\build\Desktop_Qt_6_11_1_MinGW_64_bit_Debug --target app_foundation_tests
```

Expected: compilation fails because `QueryState::Refreshing` is not defined.

- [ ] **Step 3: Add the enum and update the pane contract**

Replace the enum with:

```cpp
enum QueryState {
    Idle = 0,
    Loading = 1,
    Loaded = 2,
    Refreshing = 3,
    Empty = 4,
    Error = 5,
    LoginRequired = 6
};
```

Replace the state-facing portion of `QueryStatePane.qml` with:

```qml
property int queryState: 0
property string errorMessage: ""
property string emptyTitle: "暂无数据"
property string emptyDescription: ""
property string loginMessage: "请先登录后查看"
signal retry()
signal loginRequested()

readonly property bool firstLoading: queryState === 1
visible: firstLoading || queryState === 4 || queryState === 5 || queryState === 6
```

Bind its child views to `root.firstLoading`, `root.queryState === 4`, `=== 5`, and `=== 6`. In every consuming QML file replace `state:` with `queryState:` and remove the separate `loading:` assignment.

- [ ] **Step 4: Run the focused test and QML lint**

Run the app foundation test with the Qt and MinGW DLL directories prepended to `PATH`, then run:

```powershell
D:\QT\Tools\CMake_64\bin\cmake.exe --build D:\SCU-Nexus\SCU_Nexus\build\Desktop_Qt_6_11_1_MinGW_64_bit_Debug --target all_qmllint
```

Expected: the test passes; QML lint has zero errors and no `property override` warning for `QueryStatePane.qml`.

- [ ] **Step 5: Commit**

```bash
git add SCU_Nexus/src/common/QueryState.h SCU_Nexus/qml/components/query/QueryStatePane.qml SCU_Nexus/qml/pages/calendar/AcademicCalendarPage.qml SCU_Nexus/qml/pages/exam/ExamPlanPage.qml SCU_Nexus/qml/pages/grades/SchemeScoresTab.qml SCU_Nexus/qml/pages/grades/PassingScoresTab.qml SCU_Nexus/qml/pages/grades/CustomStatsTab.qml SCU_Nexus/tests/test_app_foundation.cpp SCU_Nexus/CMakeLists.txt
git commit -m "feat: add non-blocking query refresh state"
```

### Task 2: Persist and sanitize window geometry

**Files:**
- Modify: `SCU_Nexus/src/app/AppSettings.h`
- Modify: `SCU_Nexus/src/app/AppSettings.cpp`
- Modify: `SCU_Nexus/main.cpp`
- Modify: `SCU_Nexus/qml/App.qml`
- Test: `SCU_Nexus/tests/test_app_foundation.cpp`

**Interfaces:**
- Consumes: `QSettings`, `QGuiApplication::primaryScreen()`.
- Produces: `QVariantMap AppSettings::restoreWindowGeometry() const`; `void saveWindowGeometry(int,int,int,int)`; static `QRect sanitizedGeometry(QRect,QRect)`.

- [ ] **Step 1: Add failing geometry tests**

Add `AppSettings.{h,cpp}` to `app_foundation_tests`, then add:

```cpp
void windowGeometryUsesRequiredDefaults()
{
    const QRect actual = AppSettings::sanitizedGeometry({}, QRect(0, 0, 1920, 1080));
    QCOMPARE(actual.size(), QSize(1100, 760));
    QVERIFY(QRect(0, 0, 1920, 1080).contains(actual));
}

void windowGeometryClampsSizeAndPosition()
{
    QCOMPARE(AppSettings::sanitizedGeometry(QRect(-500, -500, 300, 200), QRect(0, 0, 1024, 700)),
             QRect(0, 0, 900, 620));
}
```

Expected before implementation: compilation fails because `sanitizedGeometry` is absent.

- [ ] **Step 2: Implement the settings API**

Declare:

```cpp
Q_INVOKABLE QVariantMap restoreWindowGeometry() const;
Q_INVOKABLE void saveWindowGeometry(int x, int y, int width, int height);
static QRect sanitizedGeometry(const QRect &saved, const QRect &available);
```

Use keys `window/geometry`; minimum size `900×620`; default size `1100×760`. Center the default rectangle, shrink it only when the available screen is smaller, and move any off-screen saved rectangle inside `available`. Return `x`, `y`, `width`, and `height` in the map.

- [ ] **Step 3: Wire AppSettings into QML**

Create one `AppSettings appSettings;` in `main.cpp`, expose it as `appSettings`, and in `App.qml` use:

```qml
Component.onCompleted: {
    const geometry = appSettings.restoreWindowGeometry()
    window.x = geometry.x
    window.y = geometry.y
    window.width = geometry.width
    window.height = geometry.height
    appController.initialize()
}

onClosing: function(close) {
    appSettings.saveWindowGeometry(window.x, window.y, window.width, window.height)
    close.accepted = true
}
```

- [ ] **Step 4: Run the foundation test and lint**

Expected: both geometry tests pass; QML lint reports no errors.

- [ ] **Step 5: Commit**

```bash
git add SCU_Nexus/src/app/AppSettings.h SCU_Nexus/src/app/AppSettings.cpp SCU_Nexus/main.cpp SCU_Nexus/qml/App.qml SCU_Nexus/tests/test_app_foundation.cpp SCU_Nexus/CMakeLists.txt
git commit -m "feat: persist safe application window geometry"
```

### Task 3: Make startup initialization observable and retryable

**Files:**
- Modify: `SCU_Nexus/src/app/AppController.h`
- Modify: `SCU_Nexus/src/app/AppController.cpp`
- Modify: `SCU_Nexus/main.cpp`
- Modify: `SCU_Nexus/qml/App.qml`
- Test: `SCU_Nexus/tests/test_app_foundation.cpp`
- Test: `SCU_Nexus/tests/test_person_b_foundation.cpp`

**Interfaces:**
- Consumes: lambdas wrapping `QueryCacheRepository::open()` and `ScheduleRepository::init()`.
- Produces: `Q_PROPERTY(QString startupError ...)`; constructor-injected `StartupStep` callbacks; idempotent `initialize()`.

- [ ] **Step 1: Add startup success and failure tests**

Add two callback types in `AppController.h` so the application layer does not force repositories to depend on app headers:

```cpp
struct StartupStepResult {
    bool ok = true;
    QString message;
};
using StartupStep = std::function<StartupStepResult()>;
```

Extend the constructor with two optional `StartupStep` arguments. In the foundation test use:

```cpp
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
controller.initialize();
QVERIFY(controller.ready());
QVERIFY(controller.startupError().isEmpty());
QCOMPARE(cacheCalls, 1);
QCOMPARE(scheduleCalls, 1);
```

For a stateful cache callback that fails once and then succeeds, assert the first call leaves `!ready()` with `startupError()` containing `查询缓存`; a second `initialize()` invokes the cache callback again, invokes the schedule callback once, and becomes ready.

- [ ] **Step 2: Run the test to verify failure**

Expected: compilation fails because `StartupStepResult`, the callback constructor parameters, and `startupError` do not exist.

- [ ] **Step 3: Implement startup orchestration**

Store the two optional callbacks after `ScuAuthService*`, and implement:

```cpp
void AppController::initialize()
{
    setReady(false);
    setStartupError({});
    if (m_queryCacheInitializer) {
        const StartupStepResult result = m_queryCacheInitializer();
        if (!result.ok) {
            failStartup(QStringLiteral("查询缓存初始化失败：%1").arg(result.message));
            return;
        }
    }
    if (m_scheduleInitializer) {
        const StartupStepResult result = m_scheduleInitializer();
        if (!result.ok) {
            failStartup(QStringLiteral("课表数据初始化失败：%1").arg(result.message));
            return;
        }
    }
    setReady(true);
    emit startupFinished();
}
```

`failStartup()` sets the property and emits `startupFailed`. In `main.cpp`, construct both repositories before `AppController`, pass lambdas that call `queryCache.open()`/`scheduleRepository.init()` and return a `StartupStepResult`, and remove the ignored `queryCache.open()` call.

- [ ] **Step 4: Render startup error and retry action**

In `App.qml`, show `LoadingView` only while `!ready && startupError.length === 0`; show `ErrorView` when the error is non-empty and call `appController.initialize()` from retry.

- [ ] **Step 5: Run app and B tests**

Expected: `app_foundation_tests` and `person_b_foundation_tests` pass; starting the app with an unwritable cache path in the fake test is observable rather than silently ready.

- [ ] **Step 6: Commit**

```bash
git add SCU_Nexus/src/app/AppController.h SCU_Nexus/src/app/AppController.cpp SCU_Nexus/main.cpp SCU_Nexus/qml/App.qml SCU_Nexus/tests/test_app_foundation.cpp SCU_Nexus/tests/test_person_b_foundation.cpp SCU_Nexus/CMakeLists.txt
git commit -m "feat: make repository startup failures retryable"
```

### Task 4: Connect real query-cache clearing to Settings

**Files:**
- Modify: `SCU_Nexus/src/viewmodels/QueryCacheViewModel.h`
- Modify: `SCU_Nexus/src/viewmodels/QueryCacheViewModel.cpp`
- Modify: `SCU_Nexus/main.cpp`
- Modify: `SCU_Nexus/qml/SettingsPage.qml`
- Test: `SCU_Nexus/tests/test_app_foundation.cpp`

**Interfaces:**
- Consumes: initialized `QueryCacheRepository`.
- Produces: `errorMessage`, `clearAll()` and `cacheCleared()` for Settings.

- [ ] **Step 1: Add a failing SQLite-backed clear test**

Use `QTemporaryDir`, open a real `QueryCacheRepository`, insert `exam-plan`, call `QueryCacheViewModel::clearAll()`, then assert the key is absent and `errorMessage()` is empty. Construct a ViewModel with `nullptr` and assert failure plus `缓存仓库未初始化`.

- [ ] **Step 2: Implement observable clear results**

Add:

```cpp
Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
QString errorMessage() const;
signals:
    void errorMessageChanged();
    void cacheCleared();
```

`clearAll()` must clear the previous error, call the repository, use `lastError()` on failure, and emit `cacheCleared()` only on success.

- [ ] **Step 3: Wire the settings action**

Expose `queryCacheViewModel` in `main.cpp`. Replace the placeholder branch with:

```qml
if (queryCacheViewModel.clearAll())
    toastManager.show("已清除考表、成绩和校历查询缓存", "success")
else
    toastManager.show(queryCacheViewModel.errorMessage, "error")
```

Change the section description to `清除本地课表或校历、考表、成绩查询缓存。`.

- [ ] **Step 4: Run test and lint, then commit**

```bash
git add SCU_Nexus/src/viewmodels/QueryCacheViewModel.h SCU_Nexus/src/viewmodels/QueryCacheViewModel.cpp SCU_Nexus/main.cpp SCU_Nexus/qml/SettingsPage.qml SCU_Nexus/tests/test_app_foundation.cpp SCU_Nexus/CMakeLists.txt
git commit -m "feat: connect settings query cache cleanup"
```

### Task 5: Close authentication logging and Cookie disclosure gaps

**Files:**
- Modify: `SCU_Nexus/src/core/logging/AuthLogger.cpp`
- Modify: `SCU_Nexus/src/core/network/CookieJar.h`
- Modify: `SCU_Nexus/src/core/network/CookieJar.cpp`
- Modify: `SCU_Nexus/src/core/network/CookieHttpClient.h`
- Modify: `SCU_Nexus/src/core/network/CookieHttpClient.cpp`
- Modify: `SCU_Nexus/src/services/auth/ScuAuthService.cpp`
- Test: `SCU_Nexus/tests/test_person_b_foundation.cpp`

**Interfaces:**
- Consumes: existing `AuthLogger` and Cookie store.
- Produces: count/name-only `cookieSummaryForDebug()` and redacted identity fields.

- [ ] **Step 1: Add failing privacy tests**

Add tests asserting:

```cpp
const QString redacted = AuthLogRedactor::apply(
    QStringLiteral("username=202312345678 studentId: 202398765432 captcha=ABCD Cookie: TGC=secret"));
QVERIFY(!redacted.contains(QStringLiteral("202312345678")));
QVERIFY(!redacted.contains(QStringLiteral("202398765432")));
QVERIFY(!redacted.contains(QStringLiteral("ABCD")));
QVERIFY(!redacted.contains(QStringLiteral("secret")));
```

Store two cookies and assert the debug summary contains their names and `count=2` but contains neither value.

- [ ] **Step 2: Implement privacy-safe summaries**

Rename `cookieHeaderForDebug()` to `cookieSummaryForDebug()` in both classes. Return deterministic text such as `count=2; names=JSESSIONID,TGC` with sorted unique names. Add case-insensitive redaction rules for `username`, `studentId`, `captcha`, `Cookie`, JSON `token`, and query/form `password`.

- [ ] **Step 3: Remove raw username logging**

Replace:

```cpp
AuthLogger::instance().info(QStringLiteral("ScuAuth"), QStringLiteral("login start user=%1").arg(username));
```

with:

```cpp
AuthLogger::instance().info(QStringLiteral("ScuAuth"), QStringLiteral("login start"));
```

- [ ] **Step 4: Run all B tests and commit**

Expected: existing redirects/Cookie tests and new privacy tests pass.

```bash
git add SCU_Nexus/src/core/logging/AuthLogger.cpp SCU_Nexus/src/core/network/CookieJar.h SCU_Nexus/src/core/network/CookieJar.cpp SCU_Nexus/src/core/network/CookieHttpClient.h SCU_Nexus/src/core/network/CookieHttpClient.cpp SCU_Nexus/src/services/auth/ScuAuthService.cpp SCU_Nexus/tests/test_person_b_foundation.cpp
git commit -m "fix: prevent sensitive authentication diagnostics"
```

### Task 6: Complete the error taxonomy and exam parsing outcome

**Files:**
- Modify: `SCU_Nexus/src/core/network/NetworkError.h`
- Modify: `SCU_Nexus/src/services/api/ZhjwParsers.h`
- Modify: `SCU_Nexus/src/services/api/ZhjwParsers.cpp`
- Modify: `SCU_Nexus/src/services/api/ZhjwApiService.cpp`
- Test: `SCU_Nexus/tests/test_person_b_foundation.cpp`

**Interfaces:**
- Produces: `ParameterInvalid`, `ConflictDetected`, `DataWriteFailed`, `EmptyResult`; `ExamPlanParseResult { items, recognized, explicitlyEmpty }`.

- [ ] **Step 1: Add failing parse-outcome tests**

Use three fixture strings: one valid `widget-box`, one containing `暂无考试安排`, and one unrelated HTML page. Assert valid returns one item and `recognized`; empty returns no items with `explicitlyEmpty`; unrelated returns neither flag.

- [ ] **Step 2: Add exact error values and parser result**

Append the four new enum members before `Unknown`. Declare:

```cpp
struct ExamPlanParseResult {
    QList<ExamPlanItemDto> items;
    bool recognized = false;
    bool explicitlyEmpty = false;
};
ExamPlanParseResult parseExamPlanResult(const QString &html);
```

Recognize a page when it contains at least one `widget-box widget-color-` block; recognize explicit empty markers `暂无考试安排`, `暂无数据`, or `没有查询到`. Keep `parseExamPlan()` as a compatibility wrapper returning `.items`.

- [ ] **Step 3: Map unrecognized pages to ParseFailed**

In `fetchExamPlan`, call the result parser. If `items` is empty and `explicitlyEmpty` is false, return `ApiErrorType::ParseFailed` with message `考试安排页面结构无法识别` and a redacted, whitespace-normalized body summary capped at 500 characters. Explicit empty returns an empty successful list.

- [ ] **Step 4: Run B tests and commit**

```bash
git add SCU_Nexus/src/core/network/NetworkError.h SCU_Nexus/src/services/api/ZhjwParsers.h SCU_Nexus/src/services/api/ZhjwParsers.cpp SCU_Nexus/src/services/api/ZhjwApiService.cpp SCU_Nexus/tests/test_person_b_foundation.cpp
git commit -m "fix: distinguish empty results from parser failures"
```

### Task 7: Remove deterministic QML lint and stale-shell issues

**Files:**
- Modify: `SCU_Nexus/qml/MainShell.qml`
- Modify: `SCU_Nexus/qml/SettingsPage.qml`
- Modify: `SCU_Nexus/qml/components/EmptyView.qml`
- Modify: `SCU_Nexus/qml/components/ErrorView.qml`
- Modify: `SCU_Nexus/qml/components/LoginRequiredView.qml`
- Modify: `SCU_Nexus/qml/LoginPage.qml`
- Modify: `SCU_Nexus/CMakeLists.txt`

**Interfaces:**
- Produces: zero layout-positioning, unused-import, and property-override warnings identified by the baseline audit.

- [ ] **Step 1: Capture the current warning classes**

Run `all_qmllint` and record the counts for `Quick.layout-positioning`, `unused imports`, and `property override` in the Task 7 execution update before editing.

- [ ] **Step 2: Fix layout-managed sizes**

For rectangles inside `RowLayout`, replace `width`/`height` with `Layout.preferredWidth`/`Layout.preferredHeight`. Apply this to the 30×30 brand tile, 9×9 login indicators, and 48×48 state icons. Remove imports that lint proves unused.

- [ ] **Step 3: Remove inactive placeholder pages and stale copy**

Delete the three `*PagePlaceholder.qml` entries from `QML_FILES` and delete those files if `rg` confirms no references. Update the login subtitle so it describes the implemented authentication flow rather than saying it will be connected later.

- [ ] **Step 4: Run lint and app startup smoke**

Expected: zero QML errors; the three targeted warning classes are zero. Start `SCU_Nexus.exe`, verify MainShell appears, navigate through five sidebar items, and close normally.

- [ ] **Step 5: Commit**

```bash
git add SCU_Nexus/qml SCU_Nexus/CMakeLists.txt
git commit -m "fix: clear actionable shell QML diagnostics"
```

### Task 8: Core checkpoint

**Files:**
- Verify only.

**Interfaces:**
- Produces: a green foundation for the schedule and query plans.

- [ ] **Step 1: Build all Debug targets**

Run the configured Windows CMake build with `--parallel`.

- [ ] **Step 2: Run focused executables with runtime PATH**

Run `app_foundation_tests.exe` and `person_b_foundation_tests.exe` with `D:\QT\6.11.1\mingw_64\bin` and `D:\QT\Tools\mingw1310_64\bin` prepended.

Expected: both exit 0 with no failed QtTest cases.

- [ ] **Step 3: Run `all_qmllint`**

Expected: exit 0, zero errors, and no warnings from the explicitly repaired categories.

- [ ] **Step 4: Record evidence**

Record command, date, exit code, and covered requirements in `SCU_Nexus/docs/qa/requirements_compliance_matrix.md` when that file is created by the QA/release plan.
