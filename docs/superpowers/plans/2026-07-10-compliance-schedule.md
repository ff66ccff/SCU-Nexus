# Schedule Compliance and Reference Migration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 使本地课表、课程 CRUD、多课表和教务导入完整满足 REQ-001～REQ-004，并迁移参考项目的旧配置兼容逻辑。

**Architecture:** 课表数据仍由 `ScheduleRepository` 持久化，新增无状态 `CourseValidator` 作为编辑 ViewModel、页面 ViewModel 和导入服务共享的业务规则。应用启动阶段初始化仓储；页面只加载已就绪数据。参考 Flutter 项目只提供旧 JSON 和默认时间段语义。

**Tech Stack:** C++20、Qt 6.11 Core/Sql/Test、QML、SQLite、CMake 3.30。

## Global Constraints

- 本计划在 `2026-07-10-compliance-core-shell-auth.md` 完成后执行。
- 只迁移正式 P0/P1 所需逻辑；不迁移参考项目的 P2 页面。
- 所有课程写入口必须执行同一组字段范围、时段和冲突校验。
- 导入替换必须在单一数据库事务中完成，失败不暴露部分数据。
- 未登录的教务导入必须进入 `LoginRequired` 行为并提供登录入口。
- 不修改或提交由行尾差异造成的既有工作树改动。

---

## File Map

- `SCU_Nexus/src/models/ScheduleConfig.{h,cpp}`: 兼容 `semesterEndDate`、旧 `sectionsPerDay` 和缺失 `timeSlots`。
- `SCU_Nexus/src/models/Course.cpp`: 修正 `excludeId` 语义。
- `SCU_Nexus/src/services/course/CourseValidator.{h,cpp}`: 唯一课程校验入口。
- `SCU_Nexus/src/viewmodels/CourseEditViewModel.cpp`: 消费共享校验。
- `SCU_Nexus/src/viewmodels/ScheduleViewModel.{h,cpp}`: 公开 CRUD 返回成功/失败且不可绕过校验。
- `SCU_Nexus/src/repositories/ScheduleRepository.{h,cpp}`: 替换时总是生成新 ID，公开初始化状态。
- `SCU_Nexus/src/viewmodels/ScheduleImportViewModel.{h,cpp}`: 登录要求和状态清理。
- `SCU_Nexus/qml/pages/schedule/SchedulePage.qml`: 未登录导入跳转和提示。
- `SCU_Nexus/main.cpp`: 同步认证状态到导入 ViewModel。
- `SCU_Nexus/tests/test_schedule.cpp`: 旧格式、校验、ID 和登录态回归。
- `SCU_Nexus/CMakeLists.txt`: 编译新增校验器。

### Task 1: Migrate legacy schedule-configuration semantics

**Files:**
- Modify: `SCU_Nexus/src/models/ScheduleConfig.h`
- Modify: `SCU_Nexus/src/models/ScheduleConfig.cpp`
- Test: `SCU_Nexus/tests/test_schedule.cpp`

**Interfaces:**
- Consumes: reference `Bugaoshan-main/lib/models/course.dart:67-123,261-349`.
- Produces: `ScheduleConfig::defaultTimeSlots(int,int,int,int,int)` and backward-compatible `fromJson()`.

- [ ] **Step 1: Add failing compatibility tests**

Add three QtTest methods:

```cpp
void testScheduleConfigReadsLegacyEndDate()
{
    const auto config = ScheduleConfig::fromJson({
        {"semesterStartDate", "2026-02-23"},
        {"semesterEndDate", "2026-06-28"}
    });
    QCOMPARE(config.totalWeeks, 18);
}

void testScheduleConfigSplitsLegacySectionsPerDay()
{
    const auto config = ScheduleConfig::fromJson({{"sectionsPerDay", 10}});
    QCOMPARE(config.morningSections, 4);
    QCOMPARE(config.afternoonSections, 5);
    QCOMPARE(config.eveningSections, 1);
}

void testScheduleConfigCreatesMissingTimeSlots()
{
    const auto config = ScheduleConfig::fromJson({
        {"morningSections", 2}, {"afternoonSections", 1}, {"eveningSections", 1},
        {"courseDuration", 45}, {"breakDuration", 10}
    });
    QCOMPARE(config.timeSlots.size(), 4);
    QCOMPARE(config.timeSlots.at(0).startTime, QTime(8, 0));
    QCOMPARE(config.timeSlots.at(2).startTime, QTime(14, 0));
    QCOMPARE(config.timeSlots.at(3).startTime, QTime(19, 0));
}
```

- [ ] **Step 2: Run the focused schedule tests**

Expected: all three new cases fail against the current defaults/empty time-slot list.

- [ ] **Step 3: Implement exact compatibility rules**

In `fromJson()`:

```cpp
if (json.contains("totalWeeks")) {
    config.totalWeeks = json.value("totalWeeks").toInt(20);
} else if (json.contains("semesterEndDate") && config.semesterStartDate.isValid()) {
    const QDate end = QDate::fromString(json.value("semesterEndDate").toString(), Qt::ISODate);
    config.totalWeeks = end.isValid() ? qMax(1, static_cast<int>((config.semesterStartDate.daysTo(end) + 6) / 7)) : 20;
} else {
    config.totalWeeks = 20;
}
```

If no modern section keys exist, split legacy totals as `morning=min(total,4)`, `afternoon=clamp(total-4,0,5)`, and `evening=max(total-9,0)`. Add `defaultTimeSlots()` that returns the Jiang'an preset for 4-5-3 and otherwise generates morning from 08:00, afternoon from 14:00, evening from 19:00 using `courseDuration` and `breakDuration`. Use it when `timeSlots` is absent or empty.

- [ ] **Step 4: Run all schedule tests and commit**

```bash
git add SCU_Nexus/src/models/ScheduleConfig.h SCU_Nexus/src/models/ScheduleConfig.cpp SCU_Nexus/tests/test_schedule.cpp
git commit -m "feat: migrate legacy schedule configuration"
```

### Task 2: Correct conflict exclusion semantics

**Files:**
- Modify: `SCU_Nexus/src/models/Course.cpp:42-64`
- Test: `SCU_Nexus/tests/test_schedule.cpp`

**Interfaces:**
- Produces: `excludeId` excludes only the receiver course being edited.

- [ ] **Step 1: Add the asymmetric regression test**

```cpp
void testCourseConflictExcludeIdOnlySkipsReceiver()
{
    Course candidate;
    candidate.id = QStringLiteral("candidate");
    candidate.name = QStringLiteral("候选课程");
    candidate.dayOfWeek = 1;
    candidate.startSection = 1;
    candidate.endSection = 2;
    candidate.startWeek = 1;
    candidate.endWeek = 16;
    candidate.weekType = WeekType::Every;
    Course existing = candidate;
    existing.id = QStringLiteral("existing");
    existing.name = QStringLiteral("已有课程");
    QVERIFY(!candidate.conflictsWith(existing, QStringLiteral("candidate")));
    QVERIFY(candidate.conflictsWith(existing, QStringLiteral("existing")));
}
```

- [ ] **Step 2: Verify the second assertion fails**

Expected: current implementation excludes either matching ID and returns false.

- [ ] **Step 3: Change the guard**

Use exactly:

```cpp
if (!excludeId.isEmpty() && id == excludeId) {
    return false;
}
```

Keep all day, section, week-range, and odd/even checks unchanged.

- [ ] **Step 4: Run the complete schedule test executable and commit**

```bash
git add SCU_Nexus/src/models/Course.cpp SCU_Nexus/tests/test_schedule.cpp
git commit -m "fix: honor course conflict exclusion contract"
```

### Task 3: Centralize course validation for every write path

**Files:**
- Create: `SCU_Nexus/src/services/course/CourseValidator.h`
- Create: `SCU_Nexus/src/services/course/CourseValidator.cpp`
- Modify: `SCU_Nexus/src/viewmodels/CourseEditViewModel.cpp`
- Modify: `SCU_Nexus/src/viewmodels/ScheduleViewModel.h`
- Modify: `SCU_Nexus/src/viewmodels/ScheduleViewModel.cpp`
- Modify: `SCU_Nexus/src/services/course/ScheduleImportService.cpp`
- Modify: `SCU_Nexus/CMakeLists.txt`
- Test: `SCU_Nexus/tests/test_schedule.cpp`

**Interfaces:**
- Produces: `CourseValidationResult CourseValidator::validate(const Course&, const ScheduleConfig&, const QList<Course>&, const QString&)`.

- [ ] **Step 1: Define failing validator and public-API tests**

Tests must assert rejection of an empty name, weekday 8, week 0, end week beyond `totalWeeks`, section 0, end section beyond `sectionsPerDay`, cross-period course, and a real conflict. Also assert:

```cpp
QVERIFY(!viewModel.addCourse(invalidMap));
QVERIFY(viewModel.errorMessage().contains(QStringLiteral("课程名")));
QCOMPARE(repo.currentCourses().size(), 0);
```

and an update that conflicts with a different course returns false without changing the stored row.

- [ ] **Step 2: Add the shared type and rules**

Create:

```cpp
struct CourseValidationResult {
    bool valid = false;
    QString message;
    QString conflictCourseId;
};

class CourseValidator final
{
public:
    static CourseValidationResult validate(const Course &course,
                                           const ScheduleConfig &config,
                                           const QList<Course> &existing = {},
                                           const QString &excludeId = {});
};
```

Implement rules in this order so the user sees a deterministic message: trimmed name; weekday 1–7; weeks within 1–`totalWeeks` and ordered; sections within 1–`sectionsPerDay` and ordered; no morning/afternoon/evening boundary crossing; conflicts after skipping `excludeId` in the existing list. Conflict text includes the existing course name, weekday and section range.

- [ ] **Step 3: Route CourseEditViewModel through the validator**

Replace its duplicated field and conflict loops with one call using `m_repo->currentScheduleConfig()`, `m_repo->currentCourses()`, and `m_editingCourse.id` in edit mode. Preserve its existing property setters and `saved` signal.

- [ ] **Step 4: Make ScheduleViewModel writes observable and safe**

Change both declarations to `Q_INVOKABLE bool`. Build the candidate, validate it, call the repository only on success, set `errorMessage` on all failure paths, clear it on success, and return the repository result. Update uses the stored course as a base so omitted map keys retain their old values.

- [ ] **Step 5: Reuse field validation during import**

Call `CourseValidator::validate(course, config)` for every parsed course. Preserve import's all-or-nothing behavior and aggregate the returned messages with course names.

- [ ] **Step 6: Run schedule tests and commit**

```bash
git add SCU_Nexus/src/services/course/CourseValidator.h SCU_Nexus/src/services/course/CourseValidator.cpp SCU_Nexus/src/viewmodels/CourseEditViewModel.cpp SCU_Nexus/src/viewmodels/ScheduleViewModel.h SCU_Nexus/src/viewmodels/ScheduleViewModel.cpp SCU_Nexus/src/services/course/ScheduleImportService.cpp SCU_Nexus/tests/test_schedule.cpp SCU_Nexus/CMakeLists.txt
git commit -m "refactor: centralize schedule course validation"
```

### Task 4: Guarantee fresh IDs for replacement imports

**Files:**
- Modify: `SCU_Nexus/src/repositories/ScheduleRepository.cpp`
- Modify: `SCU_Nexus/src/viewmodels/ScheduleImportViewModel.cpp`
- Test: `SCU_Nexus/tests/test_schedule.cpp`

**Interfaces:**
- Produces: replacement APIs regenerate every incoming course ID inside the repository transaction.

- [ ] **Step 1: Add repository-contract tests**

Create two incoming courses with non-empty IDs, replace an existing schedule, load the stored courses, and assert neither old incoming ID remains and the two stored IDs are non-empty and unique. Repeat through `replaceScheduleCoursesAndSwitch()`.

- [ ] **Step 2: Verify tests fail against preserved IDs**

Expected: current `replaceScheduleCourses()` keeps non-empty IDs.

- [ ] **Step 3: Make ID regeneration a repository decision**

Change the internal helper to:

```cpp
bool insertCourses(QSqlDatabase &db, const QString &scheduleId,
                   const QList<Course> &courses, bool regenerateIds)
```

Bind `QUuid::createUuid()` whenever `regenerateIds || course.id.isEmpty()`. Pass `true` from both replacement methods and `false` when adding a new schedule. Refactor the standalone replacement loop to use the helper inside its existing transaction.

- [ ] **Step 4: Remove caller-side UUID generation**

In `ScheduleImportViewModel::resolveConflict("updateExisting")`, pass `m_pendingCourses` directly and use its size in the status message.

- [ ] **Step 5: Run schedule tests and commit**

```bash
git add SCU_Nexus/src/repositories/ScheduleRepository.cpp SCU_Nexus/src/viewmodels/ScheduleImportViewModel.cpp SCU_Nexus/tests/test_schedule.cpp
git commit -m "fix: regenerate course ids during schedule replacement"
```

### Task 5: Model login-required import behavior

**Files:**
- Modify: `SCU_Nexus/src/viewmodels/ScheduleImportViewModel.h`
- Modify: `SCU_Nexus/src/viewmodels/ScheduleImportViewModel.cpp`
- Modify: `SCU_Nexus/main.cpp`
- Modify: `SCU_Nexus/qml/pages/schedule/SchedulePage.qml`
- Modify: `SCU_Nexus/qml/pages/schedule/ImportScheduleDialog.qml`
- Test: `SCU_Nexus/tests/test_schedule.cpp`

**Interfaces:**
- Consumes: `AppController::loginStateChanged(bool)`.
- Produces: `loggedIn`, `loginRequired`, `setLoggedIn(bool)` on the import ViewModel.

- [ ] **Step 1: Add logged-out action tests**

Construct the ViewModel with fake remote callbacks that increment counters. With `setLoggedIn(false)`, assert `loadSemesters()`, `importSchedule()`, and `syncCurrentWeek()` return false, no callback is invoked, `loginRequired()` is true, and `errorMessage()` equals `请先登录后导入教务课表`. After `setLoggedIn(true)`, assert a load invokes the fake.

- [ ] **Step 2: Implement the properties and guard**

Declare:

```cpp
Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY loginStateChanged)
Q_PROPERTY(bool loginRequired READ loginRequired NOTIFY loginStateChanged)
void setLoggedIn(bool loggedIn);
```

Use one private `ensureLoggedIn()` at the start of remote actions. Setting login false clears semesters and any in-flight presentation state; starting a new action clears stale error/status/import-complete values before setting loading.

- [ ] **Step 3: Connect AppController and route logged-out users**

Initialize from `appController.loggedIn()` and connect `loginStateChanged` in `main.cpp`. In `SchedulePage.qml`, make the import button navigate to `Login` when logged out; otherwise open and load. Keep the button enabled so the required action is discoverable. Make the sync button route to login or display an adjacent login-required hint rather than silently disabling.

- [ ] **Step 4: Add dialog fallback**

In `ImportScheduleDialog.qml`, display `LoginRequiredView` when `scheduleImportViewModel.loginRequired`, with `onLoginRequested` closing the dialog and navigating to `Login`.

- [ ] **Step 5: Run tests/lint and commit**

```bash
git add SCU_Nexus/src/viewmodels/ScheduleImportViewModel.h SCU_Nexus/src/viewmodels/ScheduleImportViewModel.cpp SCU_Nexus/main.cpp SCU_Nexus/qml/pages/schedule/SchedulePage.qml SCU_Nexus/qml/pages/schedule/ImportScheduleDialog.qml SCU_Nexus/tests/test_schedule.cpp
git commit -m "feat: expose login-required schedule import state"
```

### Task 6: Remove page-owned repository initialization

**Files:**
- Modify: `SCU_Nexus/src/repositories/ScheduleRepository.h`
- Modify: `SCU_Nexus/src/repositories/ScheduleRepository.cpp`
- Modify: `SCU_Nexus/src/viewmodels/ScheduleViewModel.cpp`
- Test: `SCU_Nexus/tests/test_schedule.cpp`

**Interfaces:**
- Consumes: core plan's `AppController::initialize()`.
- Produces: `bool ScheduleRepository::initialized() const`; `ScheduleViewModel::load()` refreshes only an initialized repository.

- [ ] **Step 1: Add initialization-boundary tests**

Before repository `init()`, call `viewModel.load()` and assert `errorMessage()` is `课表数据尚未初始化`. After `init()`, create a schedule, call `load()`, and assert normal data without reopening the SQLite connection.

- [ ] **Step 2: Expose the current flag**

Add the const getter returning `m_initialized`. Keep `init()` idempotent: if already initialized, return true without adding another Qt SQL connection.

- [ ] **Step 3: Simplify page load**

Replace the `m_repo->init()` branch with an `initialized()` guard. The remainder only updates current week, models, loading and signals.

- [ ] **Step 4: Run all schedule tests and commit**

```bash
git add SCU_Nexus/src/repositories/ScheduleRepository.h SCU_Nexus/src/repositories/ScheduleRepository.cpp SCU_Nexus/src/viewmodels/ScheduleViewModel.cpp SCU_Nexus/tests/test_schedule.cpp
git commit -m "refactor: initialize schedule storage at app startup"
```

### Task 7: Schedule checkpoint

**Files:**
- Verify only.

**Interfaces:**
- Produces: evidence for REQ-001～REQ-004.

- [ ] **Step 1: Build all Debug targets**

Run the configured Windows CMake build with `--parallel`.

- [ ] **Step 2: Run `schedule_tests.exe`**

Expected: exit 0; all original and newly added cases pass.

- [ ] **Step 3: Run `all_qmllint`**

Expected: exit 0 and zero errors.

- [ ] **Step 4: Manual smoke without real credentials**

Start the app, create two local schedules, add/edit/delete courses, trigger a conflict, switch schedules, clear local data, and verify logged-out import navigates to Login. Record each result in the QA matrix; leave the real教务 import item as `人工待验`.
