# 课表与校历来源标注及教室查询迁移 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在课表与校历页面增加可跳转的数据来源，并以 Qt 原生完整教室查询替换现有用户可见的考表入口。

**Architecture:** 远端契约由 `ZhjwApiService` 和 `ZhjwParsers` 负责，`ZhjwQueryService` 提供可替换的窄接口，`ClassroomViewModel` 管理三级导航、日期和节次筛选，QML 只负责响应式展示与用户交互。现有考表后端保留但不再从主导航加载。

**Tech Stack:** C++17、Qt 6 Core/Network/QML/Quick/QuickControls2、Qt Test、CMake/CTest。

## Global Constraints

- 课表来源必须完整显示为 `数据来自http://zhjw.scu.edu.cn/student/courseSelect/thisSemesterCurriculum/index`，URL 可由系统浏览器直接打开。
- 校历来源必须完整显示为 `数据来自https://jwc.scu.edu.cn/cdxl.htm`，URL 可由系统浏览器直接打开。
- 教室查询必须包含校区、教学楼、日期、节次筛选、教室列表和 12 节占用详情。
- QML 不得引入 `WebView`、`WebEngine` 或 `QtWebChannel`。
- 复用现有 `ScuAuthService -> ZhjwAuthService -> ZhjwApiService` 会话链。
- 保留现有考表服务、模型、ViewModel 和测试；只替换用户可见入口与主壳装配。
- 当前工作区已有用户改动，尤其是 `main.cpp`、`ApiDtos.h`、`ZhjwApiService.*`、`ZhjwParsers.*` 和 `ZhjwQueryService.*`。实施时必须保留这些改动；不得整文件回退，也不得把已有改动混入自动提交。
- 由于关键文件在任务开始前已脏，源代码任务只做差异检查，不自动提交。只有完全由本任务新建且与脏文件无耦合的文档可以单独提交。

---

## File Map

### 新建

- `SCU_Nexus/src/models/ClassroomModels.h`：占用状态映射、连续节次展开、当前节次计算。
- `SCU_Nexus/src/models/ClassroomModels.cpp`：上述纯函数实现。
- `SCU_Nexus/src/viewmodels/ClassroomViewModel.h`：QML 属性与命令接口。
- `SCU_Nexus/src/viewmodels/ClassroomViewModel.cpp`：加载、三级导航、日期和筛选状态。
- `SCU_Nexus/qml/components/SourceAttribution.qml`：统一可点击来源说明。
- `SCU_Nexus/qml/pages/classroom/ClassroomPage.qml`：教室查询主页面。
- `SCU_Nexus/qml/pages/classroom/ClassroomRoomCard.qml`：教室摘要和 12 节状态。
- `SCU_Nexus/qml/pages/classroom/ClassroomDetailDialog.qml`：教室详情。
- `SCU_Nexus/qml/pages/classroom/ClassroomDateDialog.qml`：受限日期选择。
- `SCU_Nexus/qml/pages/classroom/ClassroomPeriodDialog.qml`：起止节次选择。
- `SCU_Nexus/tests/fixtures/classroom_index.html`：校区/教学楼解析样本。
- `SCU_Nexus/tests/fixtures/classroom_query.json`：教室占用解析样本。
- `SCU_Nexus/tests/test_classroom_query.cpp`：模型与 ViewModel 单元测试。

### 修改

- `SCU_Nexus/tests/test_ui_contracts.cpp`：来源链接、导航替换和教室页面契约。
- `SCU_Nexus/src/services/api/ApiDtos.h`：教室远端 DTO。
- `SCU_Nexus/src/services/api/ZhjwParsers.h/.cpp`：HTML/JSON 解析。
- `SCU_Nexus/src/services/api/ZhjwApiService.h/.cpp`：索引 GET 与可用性 POST。
- `SCU_Nexus/src/services/zhjw/ZhjwQueryService.h/.cpp`：教室窄接口及默认失败实现。
- `SCU_Nexus/src/services/zhjw/ZhjwApiQueryService.cpp`：真实接口转发。
- `SCU_Nexus/src/app/Router.h/.cpp`：`ExamPlan` 主路由替换为 `Classroom`。
- `SCU_Nexus/main.cpp`：创建并注入 `ClassroomViewModel`。
- `SCU_Nexus/qml/AppNavigation.qml`：显示“教室查询”。
- `SCU_Nexus/qml/MainShell.qml`：加载、刷新与登录保护。
- `SCU_Nexus/qml/pages/schedule/SchedulePage.qml`：课表来源。
- `SCU_Nexus/qml/pages/calendar/AcademicCalendarPage.qml`：校历来源。
- `SCU_Nexus/CMakeLists.txt`：新源文件、QML 和测试目标。

---

### Task 1: 可点击数据来源组件与两处页面标注

**Files:**
- Create: `SCU_Nexus/qml/components/SourceAttribution.qml`
- Modify: `SCU_Nexus/qml/pages/schedule/SchedulePage.qml`
- Modify: `SCU_Nexus/qml/pages/calendar/AcademicCalendarPage.qml`
- Modify: `SCU_Nexus/tests/test_ui_contracts.cpp`
- Modify: `SCU_Nexus/CMakeLists.txt`

**Interfaces:**
- Produces: `SourceAttribution { sourceUrl: string }`，点击 URL 调用 `Qt.openUrlExternally(sourceUrl)`。
- Consumes: `Theme.fontMicro`、`Theme.mutedText`、`Theme.accent`。

- [ ] **Step 1: 写入失败的 QML 契约测试**

在 `test_ui_contracts.cpp` 增加：

```cpp
void sourceAttributionsExposeExactClickableUrls()
{
    const QString component = readUtf8(QStringLiteral("qml/components/SourceAttribution.qml"));
    const QString schedule = readUtf8(QStringLiteral("qml/pages/schedule/SchedulePage.qml"));
    const QString calendar = readUtf8(QStringLiteral("qml/pages/calendar/AcademicCalendarPage.qml"));

    QVERIFY(component.contains(QStringLiteral("数据来自")));
    QVERIFY(component.contains(QStringLiteral("Qt.openUrlExternally")));
    QVERIFY(component.contains(QStringLiteral("MouseArea")));
    QVERIFY(schedule.contains(QStringLiteral(
        "http://zhjw.scu.edu.cn/student/courseSelect/thisSemesterCurriculum/index")));
    QVERIFY(calendar.contains(QStringLiteral("https://jwc.scu.edu.cn/cdxl.htm")));
}
```

- [ ] **Step 2: 运行测试并确认 RED**

Run from `SCU_Nexus`:

```bash
cmake --preset dev
cmake --build ../out/build/dev --target ui_contract_tests
ctest --test-dir ../out/build/dev -R '^ui_contract_tests$' --output-on-failure
```

Expected: FAIL，原因是 `SourceAttribution.qml` 不存在或页面尚未包含 URL。

- [ ] **Step 3: 实现最小来源组件并接入页面**

创建 `SourceAttribution.qml`：

```qml
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../styles"

RowLayout {
    id: root
    property string sourceUrl: ""
    spacing: 0

    Text {
        text: "数据来自"
        font.pixelSize: Theme.fontMicro
        color: Theme.mutedText
    }

    Text {
        id: linkText
        text: root.sourceUrl
        font.pixelSize: Theme.fontMicro
        color: Theme.accent
        font.underline: linkMouse.containsMouse
        elide: Text.ElideRight
        Layout.fillWidth: true

        MouseArea {
            id: linkMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: Qt.openUrlExternally(root.sourceUrl)
        }
    }
}
```

把组件加入 QML 模块，并在两个页面的 `ModuleHeader` 后、业务工具栏前插入：

```qml
SourceAttribution {
    Layout.fillWidth: true
    sourceUrl: "http://zhjw.scu.edu.cn/student/courseSelect/thisSemesterCurriculum/index"
}
```

```qml
SourceAttribution {
    Layout.fillWidth: true
    sourceUrl: "https://jwc.scu.edu.cn/cdxl.htm"
}
```

- [ ] **Step 4: 运行测试并确认 GREEN**

Run:

```bash
cmake --build ../out/build/dev --target ui_contract_tests
ctest --test-dir ../out/build/dev -R '^ui_contract_tests$' --output-on-failure
```

Expected: PASS。

- [ ] **Step 5: 检查本任务差异**

Run:

```bash
git diff --check -- SCU_Nexus/qml/components/SourceAttribution.qml SCU_Nexus/qml/pages/schedule/SchedulePage.qml SCU_Nexus/qml/pages/calendar/AcademicCalendarPage.qml SCU_Nexus/tests/test_ui_contracts.cpp SCU_Nexus/CMakeLists.txt
```

Expected: 无输出；不暂存或提交，以免后续 CMake 变更被拆成不完整提交。

---

### Task 2: 教室远端 DTO、解析与 API 请求

**Files:**
- Create: `SCU_Nexus/tests/fixtures/classroom_index.html`
- Create: `SCU_Nexus/tests/fixtures/classroom_query.json`
- Modify: `SCU_Nexus/src/services/api/ApiDtos.h`
- Modify: `SCU_Nexus/src/services/api/ZhjwParsers.h`
- Modify: `SCU_Nexus/src/services/api/ZhjwParsers.cpp`
- Modify: `SCU_Nexus/src/services/api/ZhjwApiService.h`
- Modify: `SCU_Nexus/src/services/api/ZhjwApiService.cpp`
- Modify: `SCU_Nexus/tests/test_person_b_foundation.cpp`

**Interfaces:**
- Produces: `ClassroomIndexDto`、`ClassroomQueryResultDto` 及其成员 DTO。
- Produces: `bool ZhjwParsers::parseClassroomIndex(const QString&, ClassroomIndexDto*, QString*)`。
- Produces: `bool ZhjwParsers::parseClassroomQuery(const QByteArray&, ClassroomQueryResultDto*, QString*)`。
- Produces: `ZhjwApiService::fetchClassroomIndex(ClassroomIndexCallback)`。
- Produces: `ZhjwApiService::fetchClassroomAvailability(campusNumber, buildingNumber, searchDate, callback)`。

- [ ] **Step 1: 添加真实形状的 fixtures 和失败解析测试**

`classroom_index.html` 使用：

```html
<html><body>
<input id="xqList" value='[{"campusName":"江安","campusNumber":"01"},{"campusName":"望江","campusNumber":"02"}]'>
<input id="jxlList" value='[{"id":{"campusNumber":"01","teachingBuildingNumber":"101"},"teachingBuildingName":"一教"},{"id":{"campusNumber":"02","teachingBuildingNumber":"201"},"teachingBuildingName":"基教楼"}]'>
</body></html>
```

`classroom_query.json` 使用：

```json
{"classrooms":[{"classroomName":"一教A101","classroomStatusCode":"01","classroomTypeCode":"01","id":{"campusNumber":"01","classroomNumber":"A101","teachingBuildingNumber":"101"},"placeNum":"120","remark":"多媒体","sfkjy":"是"}],"classroomTime":[{"id":{"campusNumber":"01","teachingBuildingNumber":"101","classroomNumber":"A101","xq":3,"sessionstart":2},"continuingsession":2,"timestatenumber":"busy","occupancymoduleId":"06"}],"date":"2026-07-15","jxzc":20}
```

在 `PersonBFoundationTest` 增加测试槽及实现：

```cpp
#include <QFile>

void zhjwParsersReadClassroomIndexAndAvailability()
{
    QFile indexFile(QFINDTESTDATA("fixtures/classroom_index.html"));
    QVERIFY(indexFile.open(QIODevice::ReadOnly));
    ClassroomIndexDto index;
    QString error;
    QVERIFY2(ZhjwParsers::parseClassroomIndex(QString::fromUtf8(indexFile.readAll()), &index, &error), qPrintable(error));
    QCOMPARE(index.campuses.size(), 2);
    QCOMPARE(index.buildings.first().teachingBuildingNumber, QStringLiteral("101"));

    QFile queryFile(QFINDTESTDATA("fixtures/classroom_query.json"));
    QVERIFY(queryFile.open(QIODevice::ReadOnly));
    ClassroomQueryResultDto result;
    QVERIFY2(ZhjwParsers::parseClassroomQuery(queryFile.readAll(), &result, &error), qPrintable(error));
    QCOMPARE(result.classrooms.first().placeNum, 120);
    QCOMPARE(result.timeSlots.first().continuingSession, 2);
    QCOMPARE(result.teachingWeek, 20);
}

void zhjwParsersRejectMalformedClassroomPayloads()
{
    ClassroomIndexDto index;
    ClassroomQueryResultDto result;
    QString error;
    QVERIFY(!ZhjwParsers::parseClassroomIndex(QStringLiteral("<html>维护中</html>"), &index, &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(!ZhjwParsers::parseClassroomQuery(QByteArrayLiteral("[]"), &result, &error));
    QVERIFY(!error.isEmpty());
}
```

- [ ] **Step 2: 运行解析测试并确认 RED**

Run:

```bash
cmake --build ../out/build/dev --target person_b_foundation_tests
../out/build/dev/person_b_foundation_tests zhjwParsersReadClassroomIndexAndAvailability zhjwParsersRejectMalformedClassroomPayloads
```

Expected: 编译失败，缺少 DTO 和解析函数。

- [ ] **Step 3: 添加 DTO 与严格解析实现**

在 `ApiDtos.h` 增加 `#include <QList>`，并增加：

```cpp
struct ClassroomCampusDto { QString campusName; QString campusNumber; };
struct ClassroomBuildingDto { QString campusNumber; QString teachingBuildingNumber; QString teachingBuildingName; };
struct ClassroomInfoDto {
    QString classroomName;
    QString classroomStatusCode;
    QString classroomTypeCode;
    QString campusNumber;
    QString classroomNumber;
    QString teachingBuildingNumber;
    int placeNum = 0;
    QString remark;
    QString borrowable;
};
struct ClassroomTimeSlotDto {
    QString campusNumber;
    QString teachingBuildingNumber;
    QString classroomNumber;
    int weekday = 0;
    int sessionStart = 0;
    int continuingSession = 1;
    QString timeStateNumber;
    QString occupancyModuleId;
};
struct ClassroomIndexDto {
    QList<ClassroomCampusDto> campuses;
    QList<ClassroomBuildingDto> buildings;
};
struct ClassroomQueryResultDto {
    QList<ClassroomInfoDto> classrooms;
    QList<ClassroomTimeSlotDto> timeSlots;
    QString date;
    int teachingWeek = 0;
};
```

实现解析时必须：使用正则只提取 `id="xqList"`/`id="jxlList"` 的单引号 JSON；要求两者顶层为数组；要求查询结果顶层为对象且存在数组字段；嵌套 `id` 缺失或节次不在 1–12 时返回 `false` 和稳定错误文本。成功前先构建局部 DTO，最后一次性赋给输出，避免半解析状态泄漏。

- [ ] **Step 4: 运行解析测试并确认 GREEN**

Run:

```bash
cmake --build ../out/build/dev --target person_b_foundation_tests
../out/build/dev/person_b_foundation_tests zhjwParsersReadClassroomIndexAndAvailability zhjwParsersRejectMalformedClassroomPayloads
```

Expected: 两项 PASS。

- [ ] **Step 5: 先添加 API 请求失败测试**

在现有 `FakeCookieHttpClient` 记录能力上增加：

```cpp
void zhjwApiRequestsClassroomIndexAndAvailability()
{
    auto *client = new FakeCookieHttpClient();
    const QString indexUrl = QStringLiteral("http://zhjw.scu.edu.cn/student/teachingResources/classroomUseStatus/index");
    const QString queryUrl = QStringLiteral("http://zhjw.scu.edu.cn/student/teachingResources/classroomUseStatus/jasInfo");
    QFile indexFile(QFINDTESTDATA("fixtures/classroom_index.html"));
    QFile queryFile(QFINDTESTDATA("fixtures/classroom_query.json"));
    QVERIFY(indexFile.open(QIODevice::ReadOnly));
    QVERIFY(queryFile.open(QIODevice::ReadOnly));
    HttpResponse indexResponse;
    indexResponse.statusCode = 200;
    indexResponse.body = indexFile.readAll();
    indexResponse.finalUrl = QUrl(indexUrl);
    HttpResponse queryResponse;
    queryResponse.statusCode = 200;
    queryResponse.body = queryFile.readAll();
    queryResponse.finalUrl = QUrl(queryUrl);
    client->responses[indexUrl] = indexResponse;
    client->responses[queryUrl] = queryResponse;

    FakeZhjwAuthService auth(client);
    ZhjwApiService api(nullptr, &auth);
    ClassroomIndexDto index;
    ClassroomQueryResultDto result;
    ApiError error;
    api.fetchClassroomIndex([&](const ClassroomIndexDto &value, const ApiError &e) { index = value; error = e; });
    api.fetchClassroomAvailability(QStringLiteral("01"), QStringLiteral("101"), QStringLiteral("2026-07-15"),
        [&](const ClassroomQueryResultDto &value, const ApiError &e) { result = value; error = e; });

    QCOMPARE(error.type, ApiErrorType::Unknown);
    QCOMPARE(client->getUrls, QStringList{indexUrl});
    QCOMPARE(client->postUrls, QStringList{queryUrl});
    QVERIFY(client->postBodies.first().contains("xqh=01"));
    QVERIFY(client->postBodies.first().contains("jxlh=101"));
    QVERIFY(client->postBodies.first().contains("searchDate=2026-07-15"));
    QCOMPARE(client->postHeaders.first().value(QStringLiteral("X-Requested-With")), QStringLiteral("XMLHttpRequest"));
    QCOMPARE(index.campuses.size(), 2);
    QCOMPARE(result.classrooms.size(), 1);
}
```

- [ ] **Step 6: 运行 API 测试并确认 RED**

Run:

```bash
cmake --build ../out/build/dev --target person_b_foundation_tests
../out/build/dev/person_b_foundation_tests zhjwApiRequestsClassroomIndexAndAvailability
```

Expected: 编译失败，缺少两个 API 方法。

- [ ] **Step 7: 实现 API 方法并复用现有会话重试**

在头文件增加回调和方法：

```cpp
using ClassroomIndexCallback = std::function<void(const ClassroomIndexDto&, const ApiError&)>;
using ClassroomAvailabilityCallback = std::function<void(const ClassroomQueryResultDto&, const ApiError&)>;
void fetchClassroomIndex(ClassroomIndexCallback callback);
void fetchClassroomAvailability(const QString &campusNumber,
                                const QString &buildingNumber,
                                const QString &searchDate,
                                ClassroomAvailabilityCallback callback);
```

GET 使用 `htmlHeaders(zhjwBase() + "/")`；POST 使用 `QUrlQuery` 生成 `xqh`、`jxlh`、空 `jslx/jasm/zwFrom/zwTo` 和 `searchDate`，并设置：

```cpp
{
    {QStringLiteral("Accept"), QStringLiteral("application/json, text/javascript, */*; q=0.01")},
    {QStringLiteral("Content-Type"), QStringLiteral("application/x-www-form-urlencoded; charset=UTF-8")},
    {QStringLiteral("Referer"), zhjwBase() + QStringLiteral("/student/teachingResources/classroomUseStatus/index")},
    {QStringLiteral("X-Requested-With"), QStringLiteral("XMLHttpRequest")},
    {QStringLiteral("User-Agent"), NetworkSettings::defaultUserAgent()}
}
```

解析失败统一返回 `ApiErrorType::ParseFailed`；网络和会话错误原样保留。

- [ ] **Step 8: 运行 Task 2 全部测试并确认 GREEN**

Run:

```bash
cmake --build ../out/build/dev --target person_b_foundation_tests
ctest --test-dir ../out/build/dev -R '^person_b_foundation_tests$' --output-on-failure
```

Expected: PASS。

- [ ] **Step 9: 检查 Task 2 差异**

Run:

```bash
git diff --check -- SCU_Nexus/src/services/api SCU_Nexus/tests/test_person_b_foundation.cpp SCU_Nexus/tests/fixtures/classroom_index.html SCU_Nexus/tests/fixtures/classroom_query.json
```

Expected: 无输出；不提交，因为 API 文件包含任务开始前的用户改动。

---

### Task 3: 状态模型、查询窄接口与 ClassroomViewModel

**Files:**
- Create: `SCU_Nexus/src/models/ClassroomModels.h`
- Create: `SCU_Nexus/src/models/ClassroomModels.cpp`
- Create: `SCU_Nexus/src/viewmodels/ClassroomViewModel.h`
- Create: `SCU_Nexus/src/viewmodels/ClassroomViewModel.cpp`
- Create: `SCU_Nexus/tests/test_classroom_query.cpp`
- Modify: `SCU_Nexus/src/services/zhjw/ZhjwQueryService.h`
- Modify: `SCU_Nexus/src/services/zhjw/ZhjwQueryService.cpp`
- Modify: `SCU_Nexus/src/services/zhjw/ZhjwApiQueryService.cpp`
- Modify: `SCU_Nexus/CMakeLists.txt`

**Interfaces:**
- Produces: `enum class ClassroomPeriodStatus { Free, InClass, Exam, Experiment, Borrowed }`。
- Produces: `QVector<ClassroomPeriodStatus> classroomPeriodStatuses(result, classroomNumber)`，长度固定 12。
- Produces: `int currentClassroomPeriod(campusName, now)`，无课时返回 0。
- Produces: `ClassroomViewModel` 的 QML 属性：`state`、`loading`、`viewMode`、`campuses`、`buildings`、`rooms`、`selectedCampusName`、`selectedBuildingName`、`selectedDate`、`selectedDateIsToday`、`teachingWeek`、`filterPeriodStart`、`filterPeriodEnd`、`currentPeriod`、`errorMessage`。
- Produces: QML 命令 `load`、`refresh`、`selectCampus`、`selectBuilding`、`goBack`、`setSelectedDate`、`setPeriodFilter`、`filterCurrentPeriod`、`clearPeriodFilter`。

- [ ] **Step 1: 添加模型与 ViewModel 失败测试**

创建假服务并覆盖教室接口：

```cpp
class FakeClassroomQueryService final : public ZhjwQueryService {
public:
    bool loggedInValue = true;
    int indexCalls = 0;
    int queryCalls = 0;
    ClassroomIndexDto index;
    ClassroomQueryResultDto result;
    ApiError nextError;

    bool loggedIn() const override { return loggedInValue; }
    void setLoggedIn(bool value) override { loggedInValue = value; emit loggedInChanged(); }
    void fetchExamPlan(ExamPlanCallback callback) override { callback({}, {}); }
    void fetchSchemeScores(JsonCallback callback) override { callback({}, {}); }
    void fetchPassingScores(JsonCallback callback) override { callback({}, {}); }
    void fetchClassroomIndex(ClassroomIndexCallback callback) override { ++indexCalls; callback(index, std::exchange(nextError, ApiError{})); }
    void fetchClassroomAvailability(const QString &, const QString &, const QString &, ClassroomAvailabilityCallback callback) override { ++queryCalls; callback(result, std::exchange(nextError, ApiError{})); }
};
```

至少增加以下独立测试：

```cpp
void expandsContinuousPeriodsAndMapsStatuses();
void computesCurrentPeriodForCampusTimetable();
void loadsCampusThenBuildingThenRooms();
void filtersRoomsAcrossInclusivePeriodRange();
void changingDateReloadsSelectedBuilding();
void mapsLoginAndNetworkErrorsToQueryState();
void ignoresDuplicateRefreshWhileLoading();
void refreshFailureKeepsCurrentRoomsAndShowsToast();
```

关键断言：`sessionStart=2, continuingSession=2, occupancyModuleId="06"` 令第 2、3 节为 `InClass`；范围 2–3 过滤掉该教室；选择教学楼后 `queryCalls==1`；日期变化后 `queryCalls==2`；未登录为 `LoginRequired`；网络错误为 `Error`。

- [ ] **Step 2: 增加测试目标并确认 RED**

在 CMake 增加 `classroom_query_tests`，链接 `Qt6::Core` 和 `Qt6::Test`，源文件包含 `ClassroomModels.*`、`ClassroomViewModel.*`、`QueryState.*`、`ZhjwQueryService.*`、`ScheduleConfig.*`、`TimeSlot.*` 和 `ApiDtos.h`。

Run:

```bash
cmake --preset dev
cmake --build ../out/build/dev --target classroom_query_tests
```

Expected: 编译失败，缺少新模型、接口和 ViewModel。

- [ ] **Step 3: 实现纯状态模型并确认模型测试 GREEN**

头文件声明：

```cpp
enum class ClassroomPeriodStatus { Free, InClass, Exam, Experiment, Borrowed };
ClassroomPeriodStatus classroomStatusForModule(const QString &moduleId);
QString classroomStatusKey(ClassroomPeriodStatus status);
QString classroomStatusText(ClassroomPeriodStatus status);
QVector<ClassroomPeriodStatus> classroomPeriodStatuses(
    const ClassroomQueryResultDto &result, const QString &classroomNumber);
int currentClassroomPeriod(const QString &campusName, const QTime &now);
```

实现规则：默认 12 个 `Free`；为每个时段展开 `[sessionStart, sessionStart + continuingSession)` 并裁剪到 1–12；模块映射严格使用 `06/07/14/room`；当前节次复用 `ScheduleConfig::timeSlotsForCampusName`，课前 15 分钟和课间映射到即将开始的下一节，无校区预设时返回 0。

Run:

```bash
cmake --build ../out/build/dev --target classroom_query_tests
../out/build/dev/classroom_query_tests expandsContinuousPeriodsAndMapsStatuses computesCurrentPeriodForCampusTimetable
```

Expected: 两项 PASS。

- [ ] **Step 4: 扩展窄接口并保持旧假服务兼容**

在 `ZhjwQueryService` 增加：

```cpp
using ClassroomIndexCallback = std::function<void(const ClassroomIndexDto&, const ApiError&)>;
using ClassroomAvailabilityCallback = std::function<void(const ClassroomQueryResultDto&, const ApiError&)>;
virtual void fetchClassroomIndex(ClassroomIndexCallback callback);
virtual void fetchClassroomAvailability(const QString &campusNumber,
                                        const QString &buildingNumber,
                                        const QString &searchDate,
                                        ClassroomAvailabilityCallback callback);
```

这两个方法不是纯虚函数；基类默认返回：

```cpp
callback({}, {ApiErrorType::ServiceUnavailable, QStringLiteral("教室查询服务不可用")});
```

真实 `ZhjwApiQueryService` 覆盖并直接转发到 `ZhjwApiService`。这样无需修改成绩和考表测试中的既有假服务。

- [ ] **Step 5: 实现 ClassroomViewModel 最小行为**

头文件使用 `Q_OBJECT` 和上述属性；内部保存 `ClassroomIndexDto m_index`、`ClassroomQueryResultDto m_result`、校区/教学楼索引、日期、筛选和 `QueryState`。

教室 QVariant 结构固定为：

```cpp
QVariantMap{
    {QStringLiteral("name"), room.classroomName},
    {QStringLiteral("number"), room.classroomNumber},
    {QStringLiteral("seats"), room.placeNum},
    {QStringLiteral("remark"), room.remark},
    {QStringLiteral("borrowable"), room.borrowable == QStringLiteral("是")},
    {QStringLiteral("periods"), QVariantList{
        QVariantMap{{"period", 1}, {"statusKey", "free"}, {"statusText", "空闲"}}
    }}
}
```

实际 `periods` 始终生成 12 项。`rooms()` 在返回前应用闭区间筛选；未设置筛选时返回所有教室。`refresh()` 在 `Loading` 时直接返回；索引层刷新索引，教学楼/教室层刷新当前教学楼。已有教室结果时刷新失败保留 `m_result`、恢复 `Loaded` 并发出 `toastRequested`；首次查询失败才进入 `Error`。`setSelectedDate()` 只接受今天前 7 天至今天后 30 天的 ISO 日期，否则设置“日期超出可查询范围”。

- [ ] **Step 6: 运行 ViewModel 测试并确认 GREEN**

Run:

```bash
cmake --build ../out/build/dev --target classroom_query_tests
ctest --test-dir ../out/build/dev -R '^classroom_query_tests$' --output-on-failure
```

Expected: PASS。

- [ ] **Step 7: 检查 Task 3 差异**

Run:

```bash
git diff --check -- SCU_Nexus/src/models/ClassroomModels.* SCU_Nexus/src/viewmodels/ClassroomViewModel.* SCU_Nexus/src/services/zhjw SCU_Nexus/tests/test_classroom_query.cpp SCU_Nexus/CMakeLists.txt
```

Expected: 无输出；不提交，避免混入既有 `ZhjwQueryService` 改动。

---

### Task 4: 教室查询 QML、路由与应用装配

**Files:**
- Create: `SCU_Nexus/qml/pages/classroom/ClassroomPage.qml`
- Create: `SCU_Nexus/qml/pages/classroom/ClassroomRoomCard.qml`
- Create: `SCU_Nexus/qml/pages/classroom/ClassroomDetailDialog.qml`
- Create: `SCU_Nexus/qml/pages/classroom/ClassroomDateDialog.qml`
- Create: `SCU_Nexus/qml/pages/classroom/ClassroomPeriodDialog.qml`
- Modify: `SCU_Nexus/src/app/Router.h`
- Modify: `SCU_Nexus/src/app/Router.cpp`
- Modify: `SCU_Nexus/main.cpp`
- Modify: `SCU_Nexus/qml/AppNavigation.qml`
- Modify: `SCU_Nexus/qml/MainShell.qml`
- Modify: `SCU_Nexus/tests/test_ui_contracts.cpp`
- Modify: `SCU_Nexus/CMakeLists.txt`

**Interfaces:**
- Consumes: `classroomViewModel` 上下文对象和 Task 3 的全部属性/命令。
- Produces: 主路由字符串 `Classroom`，标题“教室查询”。
- Produces: QML 页面完整三级导航、日期/节次筛选和详情。

- [ ] **Step 1: 把旧考表 UI 契约替换为教室查询失败契约**

将导航测试中的 `ExamPlan` 改为 `Classroom`，并替换考表页面测试：

```cpp
void classroomReplacesExamEntryAndPreservesCompleteQueryFlow()
{
    const QString navigation = readUtf8(QStringLiteral("qml/AppNavigation.qml"));
    const QString shell = readUtf8(QStringLiteral("qml/MainShell.qml"));
    const QString page = readUtf8(QStringLiteral("qml/pages/classroom/ClassroomPage.qml"));
    const QString card = readUtf8(QStringLiteral("qml/pages/classroom/ClassroomRoomCard.qml"));
    const QString detail = readUtf8(QStringLiteral("qml/pages/classroom/ClassroomDetailDialog.qml"));

    QVERIFY(navigation.contains(QStringLiteral("教室查询")));
    QVERIFY(navigation.contains(QStringLiteral("Classroom")));
    QVERIFY(!navigation.contains(QStringLiteral("name: \"考表\"")));
    QVERIFY(shell.contains(QStringLiteral("ClassroomPage.qml")));
    QVERIFY(shell.contains(QStringLiteral("classroomViewModel.refresh")));
    QVERIFY(shell.contains(QStringLiteral("教室查询需要登录教务系统")));
    QVERIFY(page.contains(QStringLiteral("selectCampus")));
    QVERIFY(page.contains(QStringLiteral("selectBuilding")));
    QVERIFY(page.contains(QStringLiteral("setSelectedDate")));
    QVERIFY(page.contains(QStringLiteral("setPeriodFilter")));
    QVERIFY(page.contains(QStringLiteral("filterCurrentPeriod")));
    QVERIFY(page.contains(QStringLiteral("QueryStatePane")));
    QVERIFY(card.contains(QStringLiteral("periods")));
    QVERIFY(card.contains(QStringLiteral("Repeater")));
    QVERIFY(detail.contains(QStringLiteral("12")));
    QVERIFY(detail.contains(QStringLiteral("statusText")));
}
```

- [ ] **Step 2: 运行 UI 契约并确认 RED**

Run:

```bash
cmake --build ../out/build/dev --target ui_contract_tests
ctest --test-dir ../out/build/dev -R '^ui_contract_tests$' --output-on-failure
```

Expected: FAIL，`Classroom` 路由和页面尚不存在。

- [ ] **Step 3: 替换 Router 与主导航装配**

`Router::AppRoute` 用 `Classroom` 替换 `ExamPlan`；三个映射必须一致：

```cpp
case AppRoute::Classroom: return QStringLiteral("教室查询");
```

```cpp
else if (route == QStringLiteral("Classroom")) *outRoute = AppRoute::Classroom;
```

```cpp
case AppRoute::Classroom: return QStringLiteral("Classroom");
```

导航项改为：

```qml
{ name: "教室查询", route: "Classroom", icon: "\uE8B7" }
```

主壳改为：

```qml
else if (router.currentRoute === "Classroom") classroomViewModel.refresh()
```

```qml
return !appController.loggedIn && (route === "Classroom" || route === "Grades")
```

```qml
if (route === "Classroom") return "qrc:/SCU_Nexus/qml/pages/classroom/ClassroomPage.qml"
```

- [ ] **Step 4: 在 main.cpp 注入 ViewModel**

在共享 `zhjwQueryService` 后创建：

```cpp
ClassroomViewModel classroomViewModel(&zhjwQueryService);
QObject::connect(&classroomViewModel, &ClassroomViewModel::toastRequested,
                 &toastManager,
                 [&toastManager](const QString &message) {
                     toastManager.show(message, QStringLiteral("warning"));
                 });
engine.rootContext()->setContextProperty("classroomViewModel", &classroomViewModel);
```

- [ ] **Step 5: 实现页面和响应式对话框**

`ClassroomPage.qml` 在 `Component.onCompleted` 调用 `load()`；按 `viewMode` 显示三个 `ColumnLayout/ScrollView` 分支。校区与教学楼卡片使用 `MouseArea` 调用索引选择；教室层工具栏必须包含：

```qml
AppButton { text: classroomViewModel.selectedDate; onClicked: dateDialog.openFor(classroomViewModel.selectedDate) }
AppButton { visible: !classroomViewModel.selectedDateIsToday; text: "今天"; onClicked: classroomViewModel.setSelectedDate(Qt.formatDate(new Date(), "yyyy-MM-dd")) }
AppButton { visible: classroomViewModel.selectedDateIsToday && classroomViewModel.currentPeriod > 0; text: "当前节次空闲"; onClicked: classroomViewModel.filterCurrentPeriod() }
AppButton { text: classroomViewModel.filterPeriodStart > 0 ? "第 " + classroomViewModel.filterPeriodStart + "–" + classroomViewModel.filterPeriodEnd + " 节" : "节次不限"; onClicked: periodDialog.openFor(classroomViewModel.filterPeriodStart, classroomViewModel.filterPeriodEnd) }
```

`ClassroomRoomCard.qml` 对 `room.periods` 使用 12 项 `Repeater`，状态色函数固定为：

```qml
function statusColor(key) {
    if (key === "free") return Theme.success
    if (key === "inClass") return Theme.danger
    if (key === "exam") return "#D97706"
    if (key === "experiment") return "#7C3AED"
    return "#B45309"
}
```

每个标记设置 `ToolTip.text: "第 " + period + " 节 · " + statusText`。卡片点击把完整 room map 传给 `ClassroomDetailDialog.openFor(room)`。

`ClassroomDateDialog` 使用年/月/日三个 `SpinBox`，范围由今天前 7 天和后 30 天计算；确认时输出 ISO 日期信号。`ClassroomPeriodDialog` 使用两个 1–12 `SpinBox`，变更时维持 `start <= end`。详情对话框宽高使用 `Math.min`，列表固定显示 12 行并同时显示颜色和 `statusText`。

- [ ] **Step 6: 将所有新文件加入 CMake 并确认 UI GREEN**

将 C++ 源加入主可执行文件，将五个 classroom QML 和 `SourceAttribution.qml` 加入 `qt_add_qml_module`。

Run:

```bash
cmake --preset dev
cmake --build ../out/build/dev --target ui_contract_tests SCU_Nexus
ctest --test-dir ../out/build/dev -R '^ui_contract_tests$' --output-on-failure
```

Expected: 构建成功且 UI 契约 PASS。

- [ ] **Step 7: 检查 Task 4 差异**

Run:

```bash
git diff --check -- SCU_Nexus/main.cpp SCU_Nexus/src/app/Router.* SCU_Nexus/qml SCU_Nexus/tests/test_ui_contracts.cpp SCU_Nexus/CMakeLists.txt
```

Expected: 无输出；不提交，因为 `main.cpp` 含任务开始前改动。

---

### Task 5: 全量验证与完成审计

**Files:**
- Verify only: all files listed above

**Interfaces:**
- Consumes: Tasks 1–4 的完整实现。
- Produces: 构建、聚焦测试、完整测试和需求逐项证据。

- [ ] **Step 1: 运行格式和静态差异检查**

Run:

```bash
git diff --check
rg -n 'WebView|WebEngine|QtWebChannel' SCU_Nexus/qml
```

Expected: `git diff --check` 无输出；`rg` 无输出。

- [ ] **Step 2: 构建全部目标**

Run from `SCU_Nexus`:

```bash
cmake --preset dev
cmake --build ../out/build/dev
```

Expected: exit 0，无编译或链接错误。

- [ ] **Step 3: 运行聚焦测试**

Run:

```bash
ctest --test-dir ../out/build/dev -R '^(ui_contract_tests|person_b_foundation_tests|classroom_query_tests)$' --output-on-failure
```

Expected: 3/3 PASS。

- [ ] **Step 4: 运行完整测试集**

Run:

```bash
ctest --test-dir ../out/build/dev --output-on-failure
```

Expected: 100% tests passed。

- [ ] **Step 5: 做需求逐项完成审计**

Run:

```bash
rg -n 'thisSemesterCurriculum/index|Qt.openUrlExternally' SCU_Nexus/qml/pages/schedule/SchedulePage.qml SCU_Nexus/qml/components/SourceAttribution.qml
rg -n 'jwc.scu.edu.cn/cdxl.htm|Qt.openUrlExternally' SCU_Nexus/qml/pages/calendar/AcademicCalendarPage.qml SCU_Nexus/qml/components/SourceAttribution.qml
rg -n '教室查询|Classroom|classroomViewModel' SCU_Nexus/qml/AppNavigation.qml SCU_Nexus/qml/MainShell.qml SCU_Nexus/src/app/Router.* SCU_Nexus/main.cpp
rg -n 'selectCampus|selectBuilding|setSelectedDate|setPeriodFilter|filterCurrentPeriod|periods|statusText' SCU_Nexus/qml/pages/classroom SCU_Nexus/src/viewmodels/ClassroomViewModel.*
```

Expected: 每项均在对应生产文件中命中；不存在主导航 `name: "考表"` 或 `route: "ExamPlan"`。

- [ ] **Step 6: 审查工作区归属并交付**

Run:

```bash
git status --short
git diff --stat
git diff -- SCU_Nexus/main.cpp SCU_Nexus/src/services/api/ZhjwApiService.cpp SCU_Nexus/src/services/api/ZhjwApiService.h SCU_Nexus/src/services/zhjw/ZhjwQueryService.h SCU_Nexus/src/services/zhjw/ZhjwApiQueryService.cpp
```

Expected: 本任务改动与任务开始前的注释/认证改动同时保留；不暂存、不提交、不覆盖用户已有变更。最终答复列出新增功能、验证命令和工作区仍含既有改动这一事实。
