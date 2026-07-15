# Qwen Structured Calendar Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ship a Qwen-key-gated calendar experience that waits 20–60 seconds with `thinking...`, then renders locally stored structured data for every academic calendar currently listed by Sichuan University.

**Architecture:** A versioned JSON resource contains the manually audited 2021–2022 through 2026–2027 calendars. A strict C++ catalog converts it to typed calendar models and exposes the selected document through the existing `AcademicCalendarViewModel`; `AppSettings` persists the Key, while QML owns the page-lifetime timer and the three presentation states.

**Tech Stack:** C++20, Qt 6 Core/Gui/QML/Quick, QSettings, QJsonDocument, Qt Test, QML, CMake/CTest.

## Global Constraints

- Product UI must not contain “仅用于演示”“不会发送” or equivalent disclosure text.
- Parse all 6 academic years and 12 semantic terms listed at `https://jwc.scu.edu.cn/cdxl.htm` on 2026-07-15 into source-controlled structured data.
- Treat the four 2022–2023 image files as two semantic terms with duplicate resolutions, not four terms.
- Keep the academic-year selector and refresh action visible and usable in image, thinking, structured, and fallback states.
- An entered Key enables `thinking...` for a fresh random integer interval in the inclusive range 20,000–60,000 ms on each page entry.
- Do not add a Qwen network request, runtime OCR, WebView, database migration, or automatic resource writer.
- Preserve the existing public-calendar fetch, query cache, image loading, image viewer, and source attribution behavior.
- Dates in source data use ISO 8601 and all program decisions use stable English enum values rather than Chinese labels.
- Every task follows red-green-refactor and ends in an independent commit.

---

## File Map

- `SCU_Nexus/resources/calendar/academic_calendars.json`: the only source-controlled structured calendar catalog.
- `SCU_Nexus/tests/test_calendar_data.cpp`: raw resource completeness and date-integrity tests.
- `SCU_Nexus/src/models/AcademicCalendarModels.{h,cpp}`: typed year/term/week/event/note structures plus QML conversion.
- `SCU_Nexus/src/services/calendar/AcademicCalendarCatalog.{h,cpp}`: strict JSON parsing, validation, and entry lookup.
- `SCU_Nexus/tests/test_person_d_queries.cpp`: catalog and ViewModel selection regression tests.
- `SCU_Nexus/src/app/AppSettings.{h,cpp}`: Qwen Key persistence and configured-state notification.
- `SCU_Nexus/tests/test_app_foundation.cpp`: isolated QSettings persistence tests.
- `SCU_Nexus/qml/SettingsPage.qml`: password-style Key editor with save and clear actions.
- `SCU_Nexus/qml/pages/calendar/StructuredAcademicCalendarView.qml`: term, week, event, note, and original-image presentation.
- `SCU_Nexus/qml/pages/calendar/AcademicCalendarPage.qml`: image/thinking/structured orchestration and corner prompt.
- `SCU_Nexus/tests/test_ui_contracts.cpp`: UI behavior and forbidden-copy contracts.
- `SCU_Nexus/main.cpp`: catalog construction and injection.
- `SCU_Nexus/CMakeLists.txt`: sources, QML/resource packaging, and focused test target.

---

### Task 1: Add and Verify the Complete Structured Calendar Resource

**Files:**
- Create: `SCU_Nexus/tests/test_calendar_data.cpp`
- Create: `SCU_Nexus/resources/calendar/academic_calendars.json`
- Modify: `SCU_Nexus/CMakeLists.txt`

**Interfaces:**
- Consumes: the six detail pages and their twelve semantic term images from the source matrix below.
- Produces: schema-version-1 JSON with `calendars[] -> terms[] -> weeks[]/events[]/notes[]` for later catalog parsing.

**Required source matrix:**

| Academic year | Published | Detail path | Fall image | Spring image |
|---|---|---|---|---|
| 2026-2027 | 2026-07-08 | `/info/1101/10727.htm` | `/__local/8/E7/12/2F3548E5719628BB4141F3846D0_CDB03031_480AA.png` | `/__local/5/8A/8E/E91209EBD27E116771B161CCEBF_8F5B0F08_464B9.png` |
| 2025-2026 | 2025-06-24 | `/info/1101/10035.htm` | `/__local/E/AF/7B/0B6F0E05CBAFA093563D4B80D98_063B2760_3FA13.jpg` | `/__local/9/B0/79/25F4792D1F01ABE8C1A959C6B3C_6253CD73_39F95.jpg` |
| 2024-2025 | 2024-07-05 | `/info/1101/9308.htm` | `/__local/6/D6/09/B059784043EE513BBA4A21C555B_A404354A_5FD8E.jpg` | `/__local/1/3C/E9/9ECD9920A088D712D67111E195F_8DED01A4_2CECA.png` |
| 2023-2024 | 2023-06-20 | `/info/1101/8147.htm` | `/__local/4/6C/49/5D877DC7D401583A5709DD65F0A_FFFB93EF_F483.png` | `/__local/2/7B/28/B542C13B4BC14E7ED8F51AEE225_1B06AC08_100DB.png` |
| 2022-2023 | 2022-06-27 | `/info/1101/8152.htm` | `/__local/3/43/DF/C3DB35A3CE91BEA5EA302498E96_77214235_26338.jpg` | `/__local/6/93/58/4EA55A7E31E7A2361B77CD89A44_7A765565_28F3C.jpg` |
| 2021-2022 | 2021-05-24 | `/info/1101/8144.htm` | `/__local/6/1F/6E/7DA367AD1C7E42F35A667442707_3ABFA264_2D4EA.jpg` | `/__local/0/89/36/717B557D6086EE645232F51A95B_377ECBF0_33C38.jpg` |

- [ ] **Step 1: Write the raw-data test before the resource exists**

Create a QtTest that opens `SCU_NEXUS_SOURCE_DIR "/resources/calendar/academic_calendars.json"`, then asserts the exact academic-year set, two term names per year, ISO-valid dates, seven-day week spans, unique week numbers, non-reversed events, non-empty notes, and the exact source matrix. The core assertions are:

```cpp
QCOMPARE(root.value("schemaVersion").toInt(), 1);
QCOMPARE(root.value("sourceIndexUrl").toString(),
         QStringLiteral("https://jwc.scu.edu.cn/cdxl.htm"));
QCOMPARE(root.value("capturedAt").toString(), QStringLiteral("2026-07-15"));
QCOMPARE(calendars.size(), 6);
QCOMPARE(seenYears, QSet<QString>({
    "2021-2022", "2022-2023", "2023-2024",
    "2024-2025", "2025-2026", "2026-2027"}));
QCOMPARE(termCount, 12);
QCOMPARE(termNames, QSet<QString>({"秋季学期", "春季学期"}));
QVERIFY(QDate::fromString(startDate, Qt::ISODate).isValid());
QCOMPARE(start.daysTo(end), 6);
QVERIFY(!note.value("text").toString().trimmed().isEmpty());
```

Add `calendar_data_tests` to CMake with `SCU_NEXUS_SOURCE_DIR` and `Qt6::Core;Qt6::Test`.

- [ ] **Step 2: Run the focused test and verify RED**

Run:

```bash
cmake -S SCU_Nexus -B SCU_Nexus/build -DBUILD_TESTING=ON
cmake --build SCU_Nexus/build --target calendar_data_tests
ctest --test-dir SCU_Nexus/build -R '^calendar_data_tests$' --output-on-failure
```

Expected: FAIL because `academic_calendars.json` cannot be opened.

- [ ] **Step 3: Transcribe all twelve semantic term images into the resource**

Use this exact top-level shape and populate every visible week row, normalized event, and numbered note from the source matrix:

```json
{
  "schemaVersion": 1,
  "sourceIndexUrl": "https://jwc.scu.edu.cn/cdxl.htm",
  "capturedAt": "2026-07-15",
  "calendars": [
    {
      "academicYear": "2026-2027",
      "title": "2026—2027学年四川大学校历",
      "sourcePagePath": "/info/1101/10727.htm",
      "publishedAt": "2026-07-08",
      "terms": [
        {
          "id": "2026-fall",
          "name": "秋季学期",
          "startDate": "2026-08-27",
          "endDate": "2027-02-27",
          "sourceImageUrl": "https://jwc.scu.edu.cn/__local/8/E7/12/2F3548E5719628BB4141F3846D0_CDB03031_480AA.png",
          "weeks": [],
          "events": [],
          "notes": []
        },
        {
          "id": "2027-spring",
          "name": "春季学期",
          "startDate": "2027-02-25",
          "endDate": "2027-09-04",
          "sourceImageUrl": "https://jwc.scu.edu.cn/__local/5/8A/8E/E91209EBD27E116771B161CCEBF_8F5B0F08_464B9.png",
          "weeks": [],
          "events": [],
          "notes": []
        }
      ]
    }
  ]
}
```

For each visible numbered week row, add a seven-day object such as:

```json
{"weekNo":1,"phase":"teaching","startDate":"2026-08-30","endDate":"2026-09-05","label":"教学周"}
```

For each right-column note, preserve its full original text in ordered `notes`; separately add normalized date-bearing `events`. Include the five remaining academic-year objects in descending page order using the exact source matrix. The arrays must not remain empty: each term must have at least 20 weeks, one event, and one note.

- [ ] **Step 4: Run the focused test and verify GREEN**

Run:

```bash
cmake --build SCU_Nexus/build --target calendar_data_tests
ctest --test-dir SCU_Nexus/build -R '^calendar_data_tests$' --output-on-failure
```

Expected: PASS, reporting one successful `calendar_data_tests` test.

- [ ] **Step 5: Commit the complete data baseline**

```bash
git add SCU_Nexus/resources/calendar/academic_calendars.json \
        SCU_Nexus/tests/test_calendar_data.cpp SCU_Nexus/CMakeLists.txt
git commit -m "data: add structured academic calendars"
```

---

### Task 2: Add Typed Models and the Strict Calendar Catalog

**Files:**
- Modify: `SCU_Nexus/src/models/AcademicCalendarModels.h`
- Modify: `SCU_Nexus/src/models/AcademicCalendarModels.cpp`
- Create: `SCU_Nexus/src/services/calendar/AcademicCalendarCatalog.h`
- Create: `SCU_Nexus/src/services/calendar/AcademicCalendarCatalog.cpp`
- Modify: `SCU_Nexus/tests/test_person_d_queries.cpp`
- Modify: `SCU_Nexus/CMakeLists.txt`

**Interfaces:**
- Consumes: schema-version-1 `academic_calendars.json` from Task 1.
- Produces: `StructuredAcademicCalendar`, `StructuredCalendarTerm`, `StructuredCalendarWeek`, `StructuredCalendarEvent`, `StructuredCalendarNote`, and `AcademicCalendarCatalog::calendarForEntry(title, path)`.

- [ ] **Step 1: Write failing parser and lookup tests**

Add tests that load the real resource, reject malformed schema/dates, and resolve by both title and detail path:

```cpp
AcademicCalendarCatalog catalog;
QVERIFY2(catalog.load(), qPrintable(catalog.errorMessage()));
QCOMPARE(catalog.calendars().size(), 6);

const auto byTitle = catalog.calendarForEntry(
    QStringLiteral("2026-2027学年四川大学校历"), QString());
QVERIFY(byTitle.has_value());
QCOMPARE(byTitle->terms.size(), 2);

const auto byPath = catalog.calendarForEntry(
    QString(), QStringLiteral("/info/1101/8144.htm"));
QVERIFY(byPath.has_value());
QCOMPARE(byPath->academicYear, QStringLiteral("2021-2022"));
```

Also construct a temporary malformed JSON file with `schemaVersion: 2` and assert `load()` is false with a non-empty error.

- [ ] **Step 2: Run the focused tests and verify RED**

Run:

```bash
cmake --build SCU_Nexus/build --target person_d_query_tests
ctest --test-dir SCU_Nexus/build -R '^person_d_query_tests$' --output-on-failure
```

Expected: compile failure because `AcademicCalendarCatalog` and structured model types do not exist.

- [ ] **Step 3: Implement typed models and QVariant conversion**

Declare exact fields matching the JSON:

```cpp
struct StructuredCalendarWeek {
    int weekNo = -1;
    QString phase;
    QDate startDate;
    QDate endDate;
    QString label;
    QVariantMap toVariant() const;
};

struct StructuredCalendarEvent {
    QString id;
    QString type;
    QString title;
    QDate startDate;
    QDate endDate;
    bool affectsTeaching = false;
    QVariantMap toVariant() const;
};

struct StructuredCalendarNote {
    int order = 0;
    QString text;
    QVariantMap toVariant() const;
};

struct StructuredCalendarTerm {
    QString id;
    QString name;
    QDate startDate;
    QDate endDate;
    QUrl sourceImageUrl;
    QList<StructuredCalendarWeek> weeks;
    QList<StructuredCalendarEvent> events;
    QList<StructuredCalendarNote> notes;
    QVariantMap toVariant() const;
};

struct StructuredAcademicCalendar {
    QString academicYear;
    QString title;
    QString sourcePagePath;
    QDate publishedAt;
    QList<StructuredCalendarTerm> terms;
    QVariantMap toVariant() const;
};
```

- [ ] **Step 4: Implement strict catalog loading and lookup**

Expose this exact API:

```cpp
class AcademicCalendarCatalog {
public:
    explicit AcademicCalendarCatalog(
        QString source = QStringLiteral(":/calendar/academic_calendars.json"));
    bool load();
    QString errorMessage() const;
    QList<StructuredAcademicCalendar> calendars() const;
    std::optional<StructuredAcademicCalendar> calendarForEntry(
        const QString &title, const QString &path) const;
};
```

Implement fail-fast validation for required strings, ISO dates, term count, seven-day week spans, unique year/term/week IDs, non-reversed events, and recognized phase/event enum strings. Normalize title matching by extracting the `YYYY-YYYY` or `YYYY—YYYY` year token; normalize paths by stripping the `https://jwc.scu.edu.cn` origin before comparison.

Package the resource for both `SCU_Nexus` and `person_d_query_tests` with `qt_add_resources(... PREFIX "/calendar" ...)`.

- [ ] **Step 5: Run focused tests and verify GREEN**

```bash
cmake --build SCU_Nexus/build --target person_d_query_tests
ctest --test-dir SCU_Nexus/build -R '^person_d_query_tests$' --output-on-failure
```

Expected: PASS.

- [ ] **Step 6: Commit the catalog boundary**

```bash
git add SCU_Nexus/src/models/AcademicCalendarModels.* \
        SCU_Nexus/src/services/calendar/AcademicCalendarCatalog.* \
        SCU_Nexus/tests/test_person_d_queries.cpp SCU_Nexus/CMakeLists.txt
git commit -m "feat: load structured academic calendars"
```

---

### Task 3: Persist the Qwen API Key and Add the Settings Entry

**Files:**
- Modify: `SCU_Nexus/src/app/AppSettings.h`
- Modify: `SCU_Nexus/src/app/AppSettings.cpp`
- Modify: `SCU_Nexus/tests/test_app_foundation.cpp`
- Modify: `SCU_Nexus/qml/SettingsPage.qml`
- Modify: `SCU_Nexus/tests/test_ui_contracts.cpp`

**Interfaces:**
- Consumes: existing global `appSettings` QML context property.
- Produces: `qwenApiKey`, `hasQwenApiKey`, `saveQwenApiKey(QString)`, `clearQwenApiKey()`, and a password-style settings card.

- [ ] **Step 1: Write failing persistence tests**

Use `QTemporaryDir`, `QSettings::IniFormat`, and a test-only organization/application name. Assert trimming, cross-instance restoration, blank-key disablement, clear, and one notification per actual change:

```cpp
AppSettings first;
QSignalSpy changed(&first, &AppSettings::qwenApiKeyChanged);
QVERIFY(first.saveQwenApiKey(QStringLiteral("  sk-test  ")));
QCOMPARE(first.qwenApiKey(), QStringLiteral("sk-test"));
QVERIFY(first.hasQwenApiKey());
QCOMPARE(changed.count(), 1);

AppSettings restored;
QCOMPARE(restored.qwenApiKey(), QStringLiteral("sk-test"));
QVERIFY(restored.clearQwenApiKey());
QVERIFY(!restored.hasQwenApiKey());
```

- [ ] **Step 2: Run `app_foundation_tests` and verify RED**

```bash
cmake --build SCU_Nexus/build --target app_foundation_tests
ctest --test-dir SCU_Nexus/build -R '^app_foundation_tests$' --output-on-failure
```

Expected: compile failure because the Qwen Key API does not exist.

- [ ] **Step 3: Implement QSettings persistence**

Add:

```cpp
Q_PROPERTY(QString qwenApiKey READ qwenApiKey NOTIFY qwenApiKeyChanged)
Q_PROPERTY(bool hasQwenApiKey READ hasQwenApiKey NOTIFY qwenApiKeyChanged)

QString qwenApiKey() const;
bool hasQwenApiKey() const;
Q_INVOKABLE bool saveQwenApiKey(const QString &apiKey);
Q_INVOKABLE bool clearQwenApiKey();

signals:
    void qwenApiKeyChanged();
```

Store the trimmed value under `ai/qwen_api_key`, call `QSettings::sync()`, return false on a non-`NoError` status, and emit only when the effective value changes.

- [ ] **Step 4: Verify GREEN for persistence**

Run the Task 3 Step 2 command again. Expected: PASS.

- [ ] **Step 5: Write a failing QML settings contract**

Assert `SettingsPage.qml` contains `Qwen API Key`, `passwordMode: true`, `appSettings.qwenApiKey`, `saveQwenApiKey`, and `clearQwenApiKey`, while containing none of `仅用于演示`, `不会发送`, `模拟`.

- [ ] **Step 6: Run `ui_contract_tests` and verify RED**

```bash
cmake --build SCU_Nexus/build --target ui_contract_tests
ctest --test-dir SCU_Nexus/build -R '^ui_contract_tests$' --output-on-failure
```

Expected: FAIL because the settings card is absent.

- [ ] **Step 7: Add the settings card**

Insert a “智能服务” section before “外观”. Use `AppTextField { id: qwenApiKeyField; label: "Qwen API Key"; passwordMode: true; clearable: true }`, initialize it from `appSettings.qwenApiKey`, save on the primary button/Enter, clear both storage and field on the secondary button, and show success/error via `toastManager`.

- [ ] **Step 8: Run both focused targets and verify GREEN**

```bash
cmake --build SCU_Nexus/build --target app_foundation_tests ui_contract_tests
ctest --test-dir SCU_Nexus/build -R '^(app_foundation_tests|ui_contract_tests)$' --output-on-failure
```

Expected: both tests PASS.

- [ ] **Step 9: Commit settings persistence and UI**

```bash
git add SCU_Nexus/src/app/AppSettings.* SCU_Nexus/tests/test_app_foundation.cpp \
        SCU_Nexus/qml/SettingsPage.qml SCU_Nexus/tests/test_ui_contracts.cpp
git commit -m "feat: add qwen api key setting"
```

---

### Task 4: Expose the Selected Structured Calendar from the ViewModel

**Files:**
- Modify: `SCU_Nexus/src/viewmodels/AcademicCalendarViewModel.h`
- Modify: `SCU_Nexus/src/viewmodels/AcademicCalendarViewModel.cpp`
- Modify: `SCU_Nexus/tests/test_person_d_queries.cpp`
- Modify: `SCU_Nexus/main.cpp`

**Interfaces:**
- Consumes: `AcademicCalendarCatalog` from Task 2 and existing selected calendar entry.
- Produces: `structuredCalendar`, `hasStructuredCalendar`, `structuredCalendarError`, `setStructuredCalendarCatalog()`, and `structuredCalendarChanged`.

- [ ] **Step 1: Write failing ViewModel mapping tests**

Inject a loaded catalog into an `AcademicCalendarViewModel`, apply/select fixture entries for 2026–2027 and 2021–2022, then assert the `academicYear` changes and unmatched entries clear the map without disturbing `imageUrls`.

```cpp
viewModel.setStructuredCalendarCatalog(&catalog);
viewModel.selectEntry(knownIndex);
QCOMPARE(viewModel.structuredCalendar().value("academicYear").toString(),
         QStringLiteral("2021-2022"));
QVERIFY(viewModel.hasStructuredCalendar());
```

- [ ] **Step 2: Run `person_d_query_tests` and verify RED**

Use the Task 2 focused command. Expected: compile failure for missing properties/setter.

- [ ] **Step 3: Implement the ViewModel interface**

Add these declarations:

```cpp
Q_PROPERTY(QVariantMap structuredCalendar READ structuredCalendar NOTIFY structuredCalendarChanged)
Q_PROPERTY(bool hasStructuredCalendar READ hasStructuredCalendar NOTIFY structuredCalendarChanged)
Q_PROPERTY(QString structuredCalendarError READ structuredCalendarError NOTIFY structuredCalendarChanged)

void setStructuredCalendarCatalog(AcademicCalendarCatalog *catalog);
QVariantMap structuredCalendar() const;
bool hasStructuredCalendar() const;
QString structuredCalendarError() const;

signals:
    void structuredCalendarChanged();
```

Implement a private `syncStructuredCalendar()` called after catalog injection, entry application, cache restoration, and `selectEntry()`. Do not alter `reloadList()`, `reloadSelected()`, image cache keys, or pending-request behavior.

- [ ] **Step 4: Load and inject the catalog in `main.cpp`**

Construct `AcademicCalendarCatalog academicCalendarCatalog;`, call `load()` once, and call `academicCalendarViewModel.setStructuredCalendarCatalog(&academicCalendarCatalog)`. A load error stays available through the ViewModel and must not abort application startup.

- [ ] **Step 5: Verify GREEN and commit**

```bash
cmake --build SCU_Nexus/build --target person_d_query_tests SCU_Nexus
ctest --test-dir SCU_Nexus/build -R '^person_d_query_tests$' --output-on-failure
git add SCU_Nexus/src/viewmodels/AcademicCalendarViewModel.* \
        SCU_Nexus/tests/test_person_d_queries.cpp SCU_Nexus/main.cpp
git commit -m "feat: expose structured calendar data"
```

Expected: focused tests PASS and application target builds.

---

### Task 5: Build the Structured Calendar QML View

**Files:**
- Create: `SCU_Nexus/qml/pages/calendar/StructuredAcademicCalendarView.qml`
- Modify: `SCU_Nexus/CMakeLists.txt`
- Modify: `SCU_Nexus/tests/test_ui_contracts.cpp`

**Interfaces:**
- Consumes: `property var calendarData` with `terms`, `weeks`, `events`, and `notes` from Task 4.
- Produces: `signal viewOriginalRequested(string imageUrl)` and a responsive structured presentation.

- [ ] **Step 1: Write the failing structured-view QML contract**

Assert the component exists and contains `SegmentedControl`, `calendarData.terms`, `weeks`, `events`, `notes`, `weekNo`, `startDate`, `endDate`, `查看原版`, and `viewOriginalRequested`.

- [ ] **Step 2: Run `ui_contract_tests` and verify RED**

Expected: FAIL because the component does not exist.

- [ ] **Step 3: Implement the responsive component**

Use a root `Item` containing one `ScrollView` and `ColumnLayout`. Build the term segment model from `calendarData.terms`, show an overview card, then three sections:

```qml
property var calendarData: ({})
property int selectedTermIndex: 0
readonly property var terms: calendarData.terms || []
readonly property var selectedTerm: terms.length > selectedTermIndex
                                    ? terms[selectedTermIndex] : ({})
signal viewOriginalRequested(string imageUrl)
```

- Weeks: a responsive `GridLayout`/`Repeater` of cards showing `第 N 周`, formatted start/end dates, and `label`; distinguish `teaching`, `exam`, `practice`, `winter_break`, and `summer_break` with theme-backed colors and text.
- Events: chronological cards showing `title`, range, and a stable Chinese label derived from `type`.
- Notes: an expandable card whose repeater preserves `order` and full `text`.
- Original: an `AppButton` that emits the selected term's `sourceImageUrl`.

Keep widths bounded with `Math.min`, use existing `Card`, `SectionHeader`, `SegmentedControl`, `AppButton`, and Theme tokens, and add the component to `QML_FILES`.

- [ ] **Step 4: Run QML contract and application build**

```bash
cmake --build SCU_Nexus/build --target ui_contract_tests SCU_Nexus
ctest --test-dir SCU_Nexus/build -R '^ui_contract_tests$' --output-on-failure
```

Expected: PASS and build exit 0.

- [ ] **Step 5: Commit the structured view**

```bash
git add SCU_Nexus/qml/pages/calendar/StructuredAcademicCalendarView.qml \
        SCU_Nexus/tests/test_ui_contracts.cpp SCU_Nexus/CMakeLists.txt
git commit -m "feat: render structured academic calendars"
```

---

### Task 6: Orchestrate `thinking...`, Fallback, and the No-Key Prompt

**Files:**
- Modify: `SCU_Nexus/qml/pages/calendar/AcademicCalendarPage.qml`
- Modify: `SCU_Nexus/tests/test_ui_contracts.cpp`

**Interfaces:**
- Consumes: `appSettings.hasQwenApiKey`, ViewModel structured properties, the structured view, router, and existing image viewer.
- Produces: correct page-entry timer behavior and mutually exclusive content states without hiding selector/refresh.

- [ ] **Step 1: Write failing page-state contracts**

Add checks for:

```cpp
QVERIFY(page.contains(QStringLiteral("thinking...")));
QVERIFY(page.contains(QStringLiteral("20000")));
QVERIFY(page.contains(QStringLiteral("40001")));
QVERIFY(page.contains(QStringLiteral("Math.random()")));
QVERIFY(page.contains(QStringLiteral("appSettings.hasQwenApiKey")));
QVERIFY(page.contains(QStringLiteral("StructuredAcademicCalendarView")));
QVERIFY(page.contains(QStringLiteral("router.navigate(\"Settings\")")));
QVERIFY(page.contains(QStringLiteral("当前学年暂未完成智能解析")));
```

Read the `ComboBox` and `RefreshButton` blocks and assert neither has `visible:` bound to AI state. Scan the calendar QML directory for `仅用于演示`, `不会发送`, `模拟` and fail if found.

- [ ] **Step 2: Run `ui_contract_tests` and verify RED**

Expected: FAIL because the timer and states are absent.

- [ ] **Step 3: Implement page-lifetime state and timer**

Add exact state properties and startup behavior:

```qml
property bool aiEnabled: appSettings.hasQwenApiKey
property bool aiReady: false
property bool aiThinking: false
readonly property bool structuredAvailable:
    academicCalendarViewModel.hasStructuredCalendar

Component.onCompleted: {
    academicCalendarViewModel.load()
    if (root.aiEnabled) {
        root.aiThinking = true
        aiTimer.interval = 20000 + Math.floor(Math.random() * 40001)
        aiTimer.start()
    }
}

Timer {
    id: aiTimer
    repeat: false
    onTriggered: {
        root.aiThinking = false
        root.aiReady = true
    }
}
```

Do not add `visible` conditions to the existing `ComboBox` or `RefreshButton`.

- [ ] **Step 4: Implement the three content branches**

- `aiThinking`: centered BusyIndicator plus exact `thinking...`.
- `aiReady && structuredAvailable`: `StructuredAcademicCalendarView` bound to `academicCalendarViewModel.structuredCalendar`; its original-image signal opens `CalendarImageViewer`.
- otherwise: preserve the existing `ScrollView`, image cards, `QueryStatePane`, and image clicks. When AI is ready but unavailable, place “当前学年暂未完成智能解析” above the image list.

When no Key is present, anchor a compact card/button at the content area's bottom-right with copy such as “开启智能校历解析” and navigate to Settings. It must not cover the page title, selector, refresh, or source attribution.

- [ ] **Step 5: Verify GREEN and commit**

```bash
cmake --build SCU_Nexus/build --target ui_contract_tests SCU_Nexus
ctest --test-dir SCU_Nexus/build -R '^ui_contract_tests$' --output-on-failure
git add SCU_Nexus/qml/pages/calendar/AcademicCalendarPage.qml \
        SCU_Nexus/tests/test_ui_contracts.cpp
git commit -m "feat: add qwen calendar thinking flow"
```

Expected: tests PASS and application builds.

---

### Task 7: Full Verification and Requirement Audit

**Files:**
- Modify only files required to fix failures discovered by the commands below.

**Interfaces:**
- Consumes: Tasks 1–6.
- Produces: fresh build/test evidence and a clean, scoped commit history.

- [ ] **Step 1: Configure and build every target**

```bash
cmake -S SCU_Nexus -B SCU_Nexus/build -DBUILD_TESTING=ON
cmake --build SCU_Nexus/build
```

Expected: exit 0 with application and all test targets built.

- [ ] **Step 2: Run the complete test suite**

```bash
ctest --test-dir SCU_Nexus/build --output-on-failure
```

Expected: 100% tests passed, 0 failed.

- [ ] **Step 3: Run static completion checks**

```bash
git diff --check HEAD~6..HEAD
rg -n '仅用于演示|不会发送|模拟' SCU_Nexus/qml && exit 1 || true
git status --short
git log --oneline -8
```

Expected: no whitespace errors, no forbidden product copy, only the pre-existing `.codex-tmp/` audit directory remains untracked, and each completed task has its own commit.

- [ ] **Step 4: Audit every explicit requirement against source evidence**

Confirm:

- six year documents and twelve semantic terms in JSON;
- source paths/dates/images equal the Task 1 matrix;
- every term has weeks, events, and full notes;
- Key configured state controls entry behavior;
- interval formula yields exactly 20,000 through 60,000 inclusive;
- exact `thinking...` copy is visible during the timer;
- no-Key mode retains the real image calendar and corner prompt;
- year selector and refresh stay outside conditional content;
- structured mode switches immediately with the selected entry;
- no Qwen request implementation was introduced.

- [ ] **Step 5: Commit only if verification required a correction**

```bash
git add SCU_Nexus/CMakeLists.txt SCU_Nexus/src SCU_Nexus/qml \
        SCU_Nexus/tests SCU_Nexus/resources
git commit -m "fix: complete qwen calendar verification"
```

If no correction was needed, do not create an empty commit.
