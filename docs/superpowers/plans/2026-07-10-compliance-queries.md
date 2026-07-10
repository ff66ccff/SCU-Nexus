# Calendar, Exam, and Grades Compliance Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 补齐 REQ-005～REQ-010 的校历、考试、成绩、缓存和页面状态要求，使缓存刷新不遮蔽内容，并修正成绩解析与统计边界。

**Architecture:** 三个查询 ViewModel 共享 `QueryState::Refreshing` 状态机和 `QueryCacheRepository`。Service 负责网络协议、字符集和解析结果分类；ViewModel 只负责缓存、选择和展示状态；QML 仅消费稳定属性。校历图片按条目路径分别缓存，避免跨学期串图。

**Tech Stack:** C++20、Qt 6.11 Core/Network/Sql/Test、QML、SQLite、CMake 3.30。

## Global Constraints

- 本计划在核心壳与认证计划完成后执行，并消费 `QueryState::Refreshing`。
- 首次请求使用 `Loading`；已有缓存的请求使用 `Refreshing` 且内容继续可见。
- 刷新失败保留旧数据和旧时间戳，只使用 Toast 提示；无缓存才进入 Error/LoginRequired。
- 损坏缓存不是合法空结果，网络结构变化不是 Empty。
- 页面不得显示或记录完整成绩响应。
- 真实教务结果验证保留为人工待验，不使用真实账号写自动化测试。

---

## File Map

- `SCU_Nexus/src/models/GradeModels.cpp`: JSON 标量兼容和计数回退。
- `SCU_Nexus/src/repositories/QueryCacheRepository.cpp`: 清理每次操作的错误状态，便于区分 miss 与失败。
- `SCU_Nexus/src/services/calendar/AcademicCalendarService.{h,cpp}`: 请求策略、字符集、HTTP 和解析分类。
- `SCU_Nexus/src/viewmodels/AcademicCalendarViewModel.{h,cpp}`: 每条目图片缓存、稳定选择、刷新保留。
- `SCU_Nexus/src/viewmodels/ExamPlanViewModel.{h,cpp}`: Refreshing、缓存校验和完整清理。
- `SCU_Nexus/src/viewmodels/GradesViewModel.{h,cpp}`: 双查询 Refreshing、缓存校验和筛选状态清理。
- `SCU_Nexus/qml/pages/calendar/*`: 刷新提示和图片失败占位。
- `SCU_Nexus/qml/pages/exam/*`: 非阻断刷新绑定。
- `SCU_Nexus/qml/pages/grades/*`: 完整指标、原始成绩和批量选择。
- `SCU_Nexus/tests/test_person_d_queries.cpp`: 模型、服务、缓存和 ViewModel 状态测试。

### Task 1: Make grade JSON conversion lossless and counters deterministic

**Files:**
- Modify: `SCU_Nexus/src/models/GradeModels.cpp:7-108,206-241`
- Test: `SCU_Nexus/tests/test_person_d_queries.cpp`

**Interfaces:**
- Produces: internal `jsonScalarText(QJsonValue)`; server counters are preserved when present.

- [ ] **Step 1: Add failing numeric-scalar and all-failed tests**

```cpp
void parsesNumericGradeScalarsAsText()
{
    const auto item = GradeCourseItem::fromJson({
        {"courseName", "编译原理"}, {"credit", 3.5}, {"cj", 87},
        {"courseScore", 87}, {"gradePointScore", 3.7}, {"gradeName", "A-"}
    });
    QCOMPARE(item.credit, QStringLiteral("3.5"));
    QCOMPARE(item.rawScore, QStringLiteral("87"));
}

void preservesAllFailedServerCounters()
{
    const QJsonObject root{{"lnList", QJsonArray{QJsonObject{
        {"tgms", 0}, {"wtgms", 2}, {"zms", 2},
        {"cjList", QJsonArray{
            QJsonObject{{"courseName", "A"}, {"credit", "2"}, {"gradeName", "F"}},
            QJsonObject{{"courseName", "B"}, {"credit", "2"}, {"gradeName", "F"}}
        }}
    }}}};
    const auto summary = SchemeScoreSummary::fromJson(root);
    QCOMPARE(summary.passedCount, 0);
    QCOMPARE(summary.failedCount, 2);
}
```

- [ ] **Step 2: Verify failures**

Expected: numeric `credit`/`cj` become empty strings and failed count becomes 4.

- [ ] **Step 3: Implement scalar conversion and field-presence fallback**

Use:

```cpp
QString jsonScalarText(const QJsonValue &value)
{
    if (value.isString()) return value.toString();
    if (value.isDouble()) return QString::number(value.toDouble(), 'g', 15);
    if (value.isBool()) return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    return {};
}
```

Apply it to all text fields that the API may encode numerically, especially `credit` and `cj`. Recompute `passedCount` only when `tgms` is absent and `failedCount` only when `wtgms` is absent; assign recomputed local counters rather than adding to server values.

- [ ] **Step 4: Run D tests and commit**

```bash
git add SCU_Nexus/src/models/GradeModels.cpp SCU_Nexus/tests/test_person_d_queries.cpp
git commit -m "fix: preserve grade scalars and server counters"
```

### Task 2: Validate cache payloads and fully reset local state

**Files:**
- Modify: `SCU_Nexus/src/repositories/QueryCacheRepository.cpp`
- Modify: `SCU_Nexus/src/viewmodels/ExamPlanViewModel.cpp`
- Modify: `SCU_Nexus/src/viewmodels/GradesViewModel.cpp`
- Test: `SCU_Nexus/tests/test_person_d_queries.cpp`

**Interfaces:**
- Produces: malformed JSON never sets `hasCache`; clearing resets timestamps, errors and search.

- [ ] **Step 1: Add malformed-cache tests**

Put `{not-json` into each cache key, load the corresponding ViewModel while logged out, and assert it does not claim a cache and enters `LoginRequired`. Populate valid data, set a search query/error, call `clearCache()`, and assert empty data, invalid `QDateTime`, empty error/search, false cache flags, and `Idle`.

- [ ] **Step 2: Clear repository operation errors deterministically**

At the start of `open`, `put`, `get`, `remove`, and `clear`, call `m_lastError.clear()`. A missing key returns false with an empty error; a SQL failure returns false with a non-empty error.

- [ ] **Step 3: Parse cache with shape checks**

For exam cache require `QJsonParseError::NoError` and an array document. For grade caches require an object document containing an array `lnList`. On invalid payload remove that cache key, leave `hasCache` false, and set a safe diagnostic message only when no network refresh is available.

- [ ] **Step 4: Reset every related property**

Exam clear resets `m_lastUpdated` and `m_errorMessage`. Grades clear resets both timestamps, both errors and `m_searchQuery`, emitting `lastUpdatedChanged`, `error`/data signals, and `searchQueryChanged` once per changed property.

- [ ] **Step 5: Run D tests and commit**

```bash
git add SCU_Nexus/src/repositories/QueryCacheRepository.cpp SCU_Nexus/src/viewmodels/ExamPlanViewModel.cpp SCU_Nexus/src/viewmodels/GradesViewModel.cpp SCU_Nexus/tests/test_person_d_queries.cpp
git commit -m "fix: reject damaged query caches and reset state"
```

### Task 3: Preserve cached exam and grade content while refreshing

**Files:**
- Modify: `SCU_Nexus/src/viewmodels/ExamPlanViewModel.cpp`
- Modify: `SCU_Nexus/src/viewmodels/GradesViewModel.cpp`
- Modify: `SCU_Nexus/qml/pages/exam/ExamPlanPage.qml`
- Modify: `SCU_Nexus/qml/pages/grades/GradesPage.qml`
- Test: `SCU_Nexus/tests/test_person_d_queries.cpp`

**Interfaces:**
- Consumes: `QueryState::Refreshing` from the core plan.
- Produces: duplicate suppression for Loading/Refreshing and cache-preserving failure transitions.

- [ ] **Step 1: Add in-flight state tests**

Using the existing deferred fake API, preload valid cache, call `refresh()`, and assert state is `Refreshing`, cached items remain available, and a second refresh causes no second API call. Complete with an error and assert state returns to Loaded/Empty, items and timestamps are unchanged, and one Toast is emitted. Without cache, assert the same action uses `Loading` and a failure enters Error/LoginRequired.

- [ ] **Step 2: Implement the exact state choice**

Use this pattern in all three refresh methods:

```cpp
if (state == QueryState::Loading || state == QueryState::Refreshing) return;
const bool preservingCache = hasCache;
setState(preservingCache ? QueryState::Refreshing : QueryState::Loading);
```

On cached failure, do not overwrite the previous data or timestamp; restore `Empty` only when the cached collection is empty, otherwise `Loaded`, and emit a Toast. Only a successful parse updates the timestamp and writes cache.

- [ ] **Step 3: Keep content visible in QML**

Bind `QueryStatePane.queryState` to the ViewModel state. Its core contract already hides the pane for Refreshing. Show a small `BusyIndicator` or `RefreshButton.loading` when state equals 3; do not hide the list or summary.

- [ ] **Step 4: Run D tests/lint and commit**

```bash
git add SCU_Nexus/src/viewmodels/ExamPlanViewModel.cpp SCU_Nexus/src/viewmodels/GradesViewModel.cpp SCU_Nexus/qml/pages/exam/ExamPlanPage.qml SCU_Nexus/qml/pages/grades/GradesPage.qml SCU_Nexus/tests/test_person_d_queries.cpp
git commit -m "feat: retain query content during refresh"
```

### Task 4: Harden the academic-calendar network and parser boundary

**Files:**
- Modify: `SCU_Nexus/src/services/calendar/AcademicCalendarService.h`
- Modify: `SCU_Nexus/src/services/calendar/AcademicCalendarService.cpp`
- Modify: `SCU_Nexus/src/viewmodels/AcademicCalendarViewModel.cpp`
- Modify: `SCU_Nexus/CMakeLists.txt`
- Test: `SCU_Nexus/tests/test_person_d_queries.cpp`

**Interfaces:**
- Consumes: `NetworkSettings::kDefaultTimeoutMs`, `kDefaultUserAgent`, `ApiError`.
- Produces: `buildRequest(QUrl)`, robust `decodeHtml`, `calendarPageExplicitlyEmpty`, typed failure signal.

- [ ] **Step 1: Add request/charset/parser tests**

Assert a built request contains the shared User-Agent and a 15000 ms transfer timeout. Decode bytes with `text/html; charset="GB18030"; boundary=x` and `charset='utf-8'`. Assert an unrelated HTML page is not accepted as an empty calendar, while `暂无校历数据` is explicitly empty.

- [ ] **Step 2: Verify the charset test fails**

Expected: the current code passes the trailing quote/parameter to `encodingForName` and fails to select the declared codec.

- [ ] **Step 3: Build requests through one helper**

Implement:

```cpp
QNetworkRequest AcademicCalendarService::buildRequest(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", NetworkSettings::kDefaultUserAgent.toUtf8());
    request.setRawHeader("Accept", "text/html,application/xhtml+xml");
    request.setTransferTimeout(NetworkSettings::kDefaultTimeoutMs);
    return request;
}
```

On completion, classify Qt timeout/network errors, HTTP 401/403, 429, and 5xx before reading the body. Never log more than a whitespace-normalized, redacted 500-character summary.

- [ ] **Step 4: Parse charset and empty results explicitly**

Extract charset with case-insensitive regex `charset\\s*=\\s*[\"']?([^;\"'\\s]+)`. After `parseEntries`, return success-empty only for explicit markers such as `暂无校历`, `暂无数据`, or `没有查询到`; otherwise emit `ParseFailed` when no entries are recognized. Apply the same recognized-vs-empty rule to detail images.

- [ ] **Step 5: Consume typed errors in the ViewModel**

Change the failure connection to inspect `ApiErrorType`; without cache choose LoginRequired only for authentication errors, otherwise Error. With cache restore the prior state and Toast the safe message.

- [ ] **Step 6: Run D tests and commit**

```bash
git add SCU_Nexus/src/services/calendar/AcademicCalendarService.h SCU_Nexus/src/services/calendar/AcademicCalendarService.cpp SCU_Nexus/src/viewmodels/AcademicCalendarViewModel.cpp SCU_Nexus/tests/test_person_d_queries.cpp SCU_Nexus/CMakeLists.txt
git commit -m "fix: harden academic calendar transport and parsing"
```

### Task 5: Make calendar selection and image caches entry-specific

**Files:**
- Modify: `SCU_Nexus/src/viewmodels/AcademicCalendarViewModel.h`
- Modify: `SCU_Nexus/src/viewmodels/AcademicCalendarViewModel.cpp`
- Modify: `SCU_Nexus/qml/pages/calendar/AcademicCalendarPage.qml`
- Modify: `SCU_Nexus/qml/pages/calendar/CalendarImageViewer.qml`
- Test: `SCU_Nexus/tests/test_person_d_queries.cpp`

**Interfaces:**
- Produces: `imagesCacheKey(path)` and stable selection by entry path/title; image errors have visible fallback.

- [ ] **Step 1: Add selection and cache-isolation tests**

Cache two calendar entries and different image lists under their paths. Select the second, refresh entries in reverse order, and assert the second remains selected. Select each entry and assert only its own images load. Insert malformed entries/images JSON and assert it is removed rather than treated as empty valid cache.

- [ ] **Step 2: Derive a per-entry cache key**

Use a SHA-256 hex digest of `entry.path.toUtf8()`:

```cpp
QString AcademicCalendarViewModel::imagesCacheKey(const QString &path)
{
    return QStringLiteral("academic_calendar.images.%1")
        .arg(QString::fromLatin1(QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Sha256).toHex()));
}
```

Read/write/remove the selected entry's key rather than global `academic_calendar.images`.

- [ ] **Step 3: Preserve a stable selection**

Before replacing entries, capture the selected path and title. Find the same path in new entries, then title, then fall back to index 0. Persist both path and title. Emit `selectedChanged` only after the final index is known.

- [ ] **Step 4: Apply Refreshing and write-failure behavior**

Entry/detail requests with visible cache use Refreshing. If `put()` fails, retain network data but emit a warning Toast with `lastError`; do not claim the data was persisted. Clearing removes entries, selection and every known per-entry image key, resets timestamp/error and state.

- [ ] **Step 5: Add visible image-load fallback**

In `CalendarImageViewer.qml`, add a centered `BusyIndicator` for `Image.Loading` and an `ErrorView`/text for `Image.Error` with message `校历图片加载失败，请检查网络后重试。`; keep the dialog closable.

- [ ] **Step 6: Run D tests/lint and commit**

```bash
git add SCU_Nexus/src/viewmodels/AcademicCalendarViewModel.h SCU_Nexus/src/viewmodels/AcademicCalendarViewModel.cpp SCU_Nexus/qml/pages/calendar/AcademicCalendarPage.qml SCU_Nexus/qml/pages/calendar/CalendarImageViewer.qml SCU_Nexus/tests/test_person_d_queries.cpp
git commit -m "fix: isolate academic calendar image caches"
```

### Task 6: Complete documented grade presentation and selection actions

**Files:**
- Modify: `SCU_Nexus/qml/pages/grades/SchemeScoresTab.qml`
- Modify: `SCU_Nexus/qml/pages/grades/PassingScoresTab.qml`
- Modify: `SCU_Nexus/qml/pages/grades/CustomStatsTab.qml`
- Modify: `SCU_Nexus/qml/pages/grades/GradeCourseCard.qml`
- Test: `SCU_Nexus/tests/test_person_d_queries.cpp`

**Interfaces:**
- Consumes: existing summary keys and group `passedCount`/`credits`.
- Produces: all documented metrics plus visible/current-term selection controls.

- [ ] **Step 1: Add model-level evidence assertions**

Extend D tests to assert scheme summary contains `electiveCredits` and `optionalCredits`, passing and term group maps contain `passedCount` and `credits`, and custom stats contains `failedCount` for a mixed selection.

- [ ] **Step 2: Display all summary/group fields**

Add `选修学分` and `任选学分` metrics to Scheme. Render term headers as `label + " · 通过 " + passedCount + " 门 · " + credits + " 学分"` for scheme and passing groups.

- [ ] **Step 3: Display raw score without hiding grade labels**

Change the course detail row to include `原始成绩 ` plus `course.rawScore` when non-empty, followed by normalized score and GPA. Do not replace `gradeName`; both representations remain visible.

- [ ] **Step 4: Add deterministic bulk-selection helpers**

In `CustomStatsTab.qml`, implement:

```qml
function keysForGroups(sourceGroups) {
    var keys = []
    for (var i = 0; i < sourceGroups.length; ++i)
        for (var j = 0; j < sourceGroups[i].items.length; ++j)
            keys.push(sourceGroups[i].items[j].key)
    return keys
}

function setKeysSelected(keys, selected) {
    var next = root.selectedKeys.slice()
    for (var i = 0; i < keys.length; ++i) {
        var index = next.indexOf(keys[i])
        if (selected && index < 0) next.push(keys[i])
        if (!selected && index >= 0) next.splice(index, 1)
    }
    root.selectedKeys = next
}
```

Add `全选当前筛选` and `取消当前筛选`, plus one `全选/取消本学期` action per group. Add `未通过门数` to custom metrics. Selection operations use `root.groups`, so search and attribute filters define “current”.

- [ ] **Step 5: Run D tests and QML lint, then commit**

```bash
git add SCU_Nexus/qml/pages/grades/SchemeScoresTab.qml SCU_Nexus/qml/pages/grades/PassingScoresTab.qml SCU_Nexus/qml/pages/grades/CustomStatsTab.qml SCU_Nexus/qml/pages/grades/GradeCourseCard.qml SCU_Nexus/tests/test_person_d_queries.cpp
git commit -m "feat: complete grade metrics and bulk selection"
```

### Task 7: Expand the formal D-module regression matrix

**Files:**
- Modify: `SCU_Nexus/tests/test_person_d_queries.cpp`
- Create: `SCU_Nexus/tests/fixtures/calendar_entries.html`
- Create: `SCU_Nexus/tests/fixtures/calendar_empty.html`
- Create: `SCU_Nexus/tests/fixtures/scheme_scores.json`
- Create: `SCU_Nexus/tests/fixtures/passing_scores.json`

**Interfaces:**
- Produces: fixture-backed coverage for parser, sort, GPA, state, cache and selection requirements.

- [ ] **Step 1: Replace inline large payloads with fictional fixtures**

Fixtures use fictional course names and no student IDs, tokens, cookies or real grades. Load through `QFINDTESTDATA("fixtures/<name>")` and fail explicitly if a fixture cannot be opened.

- [ ] **Step 2: Add the missing cases**

Add named QtTest slots for: quoted charset and fallback; calendar explicit empty vs parse failure; calendar cached-refresh failure retention; exam empty state and logged-out no-cache; passing parser/sort; required GPA; search and attribute filtering; current-filter selection key set; all-failed counters; numeric scalar fields; and Loading vs Refreshing duplicate suppression.

- [ ] **Step 3: Run the full D executable**

Expected: every slot passes and the executable exits 0.

- [ ] **Step 4: Commit**

```bash
git add SCU_Nexus/tests/test_person_d_queries.cpp SCU_Nexus/tests/fixtures
git commit -m "test: cover formal calendar exam and grade requirements"
```

### Task 8: Query checkpoint

**Files:**
- Verify only.

**Interfaces:**
- Produces: evidence for REQ-005～REQ-010.

- [ ] **Step 1: Build all Debug targets**

Run the Windows CMake build with `--parallel`.

- [ ] **Step 2: Run `person_d_query_tests.exe` and `person_b_foundation_tests.exe`**

Expected: both exit 0; B remains green after calendar adopts shared network/error types.

- [ ] **Step 3: Run `all_qmllint`**

Expected: exit 0 and zero errors.

- [ ] **Step 4: Manual cached-state smoke**

With fictional/local cached data, navigate among 校历、考表、成绩, trigger refresh failure, and verify content remains visible with warning feedback. Record real-campus queries as `人工待验`.
