# 人员 D：校历、考表、教务成绩查询详细任务书

## 0. 本文档定位

人员 D 负责第一阶段三个查询类模块：

- 校历查询
- 考表查询
- 教务成绩

体测查询不进入本阶段。人员 D 不需要实现体测页、体测通知、体测成绩、FitnessViewModel 或 FitnessApiService 接入。

本阶段目标是把 Flutter 版已有逻辑迁移到 Qt 桌面端，并按分层规则拆出 ViewModel、缓存、页面状态和成绩统计模型。

## 1. 迁移依据

校历：

- `Bugaoshan/lib/pages/campus/academic_calendar/academic_calendar_page.dart`
- `Bugaoshan/lib/utils/holiday_utils.dart`

考表：

- `Bugaoshan/lib/pages/campus/exam_plan/exam_plan_page.dart`
- `Bugaoshan/lib/pages/campus/exam_plan/models/exam_info.dart`
- `Bugaoshan/lib/services/api/zhjw_api_service.dart`
- `Bugaoshan/test/exam_plan_test.dart`

成绩：

- `Bugaoshan/lib/providers/grades_provider.dart`
- `Bugaoshan/lib/models/scheme_score.dart`
- `Bugaoshan/lib/pages/campus/grades/grades_page.dart`
- `Bugaoshan/lib/pages/campus/grades/scheme_scores_tab.dart`
- `Bugaoshan/lib/pages/campus/grades/passing_scores_tab.dart`
- `Bugaoshan/lib/pages/campus/grades/custom_stats_tab.dart`
- `Bugaoshan/lib/services/api/zhjw_api_service.dart`

公共状态：

- `Bugaoshan/lib/widgets/common/loading_widgets.dart`
- `Bugaoshan/lib/widgets/common/login_required_widget.dart`
- `Bugaoshan/lib/widgets/common/retryable_error_widget.dart`
- `Bugaoshan/lib/widgets/common/stat_item.dart`

## 2. 推荐文件归属

人员 D 拥有：

```text
SCU_Nexus/src/
  models/
    AcademicCalendarModels.h
    AcademicCalendarModels.cpp
    ExamPlanModels.h
    ExamPlanModels.cpp
    GradeModels.h
    GradeModels.cpp
  repositories/
    QueryCacheRepository.h
    QueryCacheRepository.cpp
  services/
    calendar/
      AcademicCalendarService.h
      AcademicCalendarService.cpp
      HolidayService.h
      HolidayService.cpp
    grades/
      GradeStatisticsService.h
      GradeStatisticsService.cpp
  viewmodels/
    AcademicCalendarViewModel.h
    AcademicCalendarViewModel.cpp
    ExamPlanViewModel.h
    ExamPlanViewModel.cpp
    GradesViewModel.h
    GradesViewModel.cpp
    QueryCacheViewModel.h
    QueryCacheViewModel.cpp
  qml/
    pages/
      calendar/
        AcademicCalendarPage.qml
        CalendarImageViewer.qml
      exam/
        ExamPlanPage.qml
        ExamCard.qml
      grades/
        GradesPage.qml
        SchemeScoresTab.qml
        PassingScoresTab.qml
        CustomStatsTab.qml
        GradeCourseCard.qml
        GradeSummaryPanel.qml
    components/
      query/
        LastUpdatedLabel.qml
        RefreshButton.qml
        QueryStatePane.qml
```

人员 D 不拥有：

- 登录页 UI。
- SCU 统一认证。
- 教务 SSO。
- 课表数据库。
- 体测页面或体测 API。
- Windows 打包。

## 3. 查询类页面统一状态

三个页面必须统一状态名，便于 A 的公共组件复用：

```cpp
enum class QueryState {
    Idle,
    Loading,
    Loaded,
    Empty,
    Error,
    LoginRequired
};
```

ViewModel 统一属性：

```cpp
Q_PROPERTY(QueryState state READ state NOTIFY stateChanged)
Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)
Q_PROPERTY(QDateTime lastUpdated READ lastUpdated NOTIFY lastUpdatedChanged)
Q_PROPERTY(bool hasCache READ hasCache NOTIFY cacheChanged)
```

统一方法：

```cpp
Q_INVOKABLE void load();
Q_INVOKABLE void refresh();
Q_INVOKABLE void clearCache();
```

状态规则：

- 首次进入没有缓存：显示 Loading 或引导获取数据。
- 首次进入失败：显示 Error。
- 有缓存时：先显示缓存，再允许用户刷新。
- 后台刷新失败：保留旧数据，Toast 显示错误。
- 用户手动刷新失败：保留旧数据，并显示错误提示。
- 未登录：考表和成绩显示 LoginRequired；校历不显示 LoginRequired。

## 4. QueryCacheRepository

### 4.1 职责

缓存查询结果，避免每次打开页面都必须请求网络。

缓存内容：

- 校历年份列表和图片 URL。
- 上次选择的校历年份。
- 考表 JSON。
- 方案成绩 JSON。
- 及格成绩 JSON。
- 每项 lastUpdated。

### 4.2 推荐存储

第一阶段可选两种方案：

1. `QSettings` 保存小 JSON。
2. SQLite 中新增 query cache 表。

建议用 SQLite，便于统一清理：

```sql
CREATE TABLE query_cache (
  cache_key TEXT PRIMARY KEY,
  payload_json TEXT NOT NULL,
  updated_at TEXT NOT NULL
);
```

cache_key 建议：

```text
academic_calendar.entries
academic_calendar.selected_year
exam_plan.latest
grades.scheme_scores
grades.passing_scores
```

D 拥有 query cache；C 的课表数据不要写入这张表。

## 5. 校历查询

### 5.1 业务定位

Flutter 版校历页面抓取四川大学教务处公开校历页面，并展示图片。Qt 第一阶段以此为主逻辑。

校历不依赖：

- SCU 登录。
- 教务 SSO。
- Cookie。

### 5.2 AcademicCalendarService

基础地址：

```text
https://jwc.scu.edu.cn
```

列表页：

```text
GET https://jwc.scu.edu.cn/cdxl.htm
```

解析：

```regex
<a[^>]+href="(info/1101/\d+\.htm)"[^>]*>[^<]*?(\d{4})-(\d{4})[^<]*</a>
```

返回：

```cpp
struct AcademicCalendarEntry {
    QString title; // 例如 2025-2026
    QString path;  // 例如 info/1101/1234.htm
};
```

详情页：

```text
GET https://jwc.scu.edu.cn/<entry.path>
```

解析图片：

```regex
<img[^>]+src="(/__local/[^"]+\.(?:jpg|jpeg|png|gif|webp))"[^>]*>
```

返回：

```cpp
struct AcademicCalendarDetail {
    AcademicCalendarEntry entry;
    QStringList imageUrls;
};
```

### 5.3 编码处理

Flutter 使用 `latin1.decode(resp.bodyBytes)`，因为教务处页面编码不一定是 UTF-8。

Qt 版建议：

1. 先检查 HTTP header charset。
2. 如果未声明，尝试 UTF-8。
3. 如果 UTF-8 失败，尝试 `GB18030` 或系统本地编码。
4. 解析失败时记录原始前 500 字节。

不要直接假设所有页面是 UTF-8。

### 5.4 AcademicCalendarViewModel

属性：

```cpp
Q_PROPERTY(QVariantList entries READ entries NOTIFY entriesChanged)
Q_PROPERTY(int selectedIndex READ selectedIndex NOTIFY selectedChanged)
Q_PROPERTY(QString selectedTitle READ selectedTitle NOTIFY selectedChanged)
Q_PROPERTY(QStringList imageUrls READ imageUrls NOTIFY imageUrlsChanged)
```

方法：

```cpp
Q_INVOKABLE void selectEntry(int index);
Q_INVOKABLE void reloadList();
Q_INVOKABLE void reloadSelected();
```

加载流程：

1. 读取缓存 entries 和 selected。
2. 如果有缓存，先显示。
3. 请求列表页。
4. 解析 entries。
5. 默认选第一项，除非缓存 selected 仍存在。
6. 请求详情页。
7. 解析 imageUrls。
8. 写缓存。

### 5.5 页面结构

`AcademicCalendarPage.qml`：

- 顶部：年份下拉框、刷新按钮。
- 主体：图片列表。
- 图片点击：打开大图查看。
- 空状态：无校历图片。
- 错误状态：加载失败，可重试。

图片加载要求：

- 宽度适配窗口。
- 保持图片比例。
- 图片加载失败显示破图占位。
- 多张图按顺序显示。
- 不把图片拉伸变形。

## 6. 考表查询

### 6.1 数据来源

人员 B 提供：

```cpp
QFuture<QList<ExamPlanItemDto>> ZhjwApiService::fetchExamPlan();
```

D 不直接请求教务 URL，不解析教务登录状态。

### 6.2 ExamPlanItem

D 可把 B 的 DTO 转为页面模型：

```cpp
struct ExamPlanItem {
    QString courseName;
    QString week;
    QString date;
    QString weekday;
    QString timeRange;
    QString location;
    QString seatNumber;
    QString ticketNumber;
    QString tip;
    bool isPast;
    QDateTime startDateTime;
    QDateTime endDateTime;
};
```

`isPast` 规则复刻 Flutter `ExamInfo.isPast`：

1. 解析 `date` 为 `QDate`。
2. 解析 `timeRange` 的结束时间。
3. 如果成功，当前时间晚于考试结束时间则 true。
4. 如果时间解析失败，当前日期晚于考试日期 + 1 天则 true。
5. 解析失败则 false。

### 6.3 排序

必须支持按时间排序：

1. 能解析日期和时间的考试按开始时间升序。
2. 已结束考试可以仍按时间位置显示，不强制置底。
3. 无法解析时间的考试放在列表末尾。
4. 同一时间按课程名排序。

### 6.4 ExamPlanViewModel

属性：

```cpp
Q_PROPERTY(int count READ count NOTIFY examsChanged)
Q_PROPERTY(QVariantList exams READ exams NOTIFY examsChanged)
Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY authChanged)
```

方法：

```cpp
Q_INVOKABLE void load();
Q_INVOKABLE void refresh();
Q_INVOKABLE QVariantMap examAt(int index) const;
```

加载流程：

1. 检查 B 的 AuthViewModel 或 AuthService 是否已登录。
2. 未登录：state = LoginRequired。
3. 有缓存：先显示缓存。
4. 调用 B 的 `fetchExamPlan()`。
5. 转模型。
6. 排序。
7. 写缓存。
8. 空列表：state = Empty。

### 6.5 页面结构

`ExamPlanPage.qml`：

- 顶部：刷新按钮、上次更新时间。
- 主体：考试卡片列表。
- 空状态：暂无考试安排。
- 未登录：LoginRequiredView。
- 错误：ErrorView。

考试卡片显示：

- 课程名。
- 周次。
- 日期。
- 星期。
- 时间。
- 地点。
- 座位号。
- 准考证号，若为空不显示。
- 提示信息，若为 `无` 不显示。
- 已结束考试弱化显示。

### 6.6 不做内容

第一阶段不做：

- 导出 ICS。
- 写入系统日历。
- 复制 JSON 到剪贴板。

这些在 Flutter 版存在相关能力，但不属于本阶段四个功能的核心闭环。

## 7. 教务成绩

### 7.1 数据来源

人员 B 提供：

```cpp
QFuture<QJsonObject> ZhjwApiService::fetchSchemeScores();
QFuture<QJsonObject> ZhjwApiService::fetchPassingScores();
```

D 不直接请求成绩 URL，不提取 callback URL，不处理教务 Cookie。

### 7.2 成绩页面结构

`GradesPage.qml` 使用三 Tab：

1. 方案成绩
2. 及格成绩
3. 自定义统计

顶部能力：

- 搜索课程名。
- 刷新当前 Tab 对应数据。
- 显示上次更新时间。

登录规则：

- 未登录显示 LoginRequiredView。
- 登录后允许加载成绩。
- session 过期但已有缓存时继续显示旧数据，同时提示登录失效。

### 7.3 GradeModels

复刻 Flutter `scheme_score.dart`。

```cpp
struct GradeCourseItem {
    QString courseName;
    QString englishCourseName;
    QString courseAttributeName; // 必修/选修/任选
    QString credit;
    QString rawScore;            // cj
    double courseScore = 0.0;
    double gradePointScore = 0.0;
    QString gradeName;           // A/B+/F 等
    QString academicYearCode;
    QString termName;            // 秋/春
    bool passed = false;
    bool hasEffectiveScore = false;
};
```

字段映射：

```text
courseName              -> courseName
englishCourseName       -> englishCourseName
courseAttributeName     -> courseAttributeName
credit                  -> credit
cj                      -> rawScore
courseScore             -> courseScore
gradePointScore         -> gradePointScore
gradeName               -> gradeName
academicYearCode        -> academicYearCode
termName                -> termName
```

规则：

- `passed = gradeName != "F" && !gradeName.isEmpty()`。
- `hasEffectiveScore = courseScore >= 0 && gradePointScore >= 0`。
- `credit` 用字符串保存，统计时再 `toDouble()`。

### 7.4 SchemeScoreSummary

对应方案成绩。

```cpp
struct SchemeScoreSummary {
    double totalCredits;      // zxf
    double earnedPlanCredits; // yxxf
    int passedCount;          // tgms
    int failedCount;          // wtgms
    int totalCount;           // zms
    QString scoreType;        // cjlx
    QList<GradeCourseItem> items;
};
```

解析：

- 从根 JSON 的 `lnList` 取第一项。
- 从第一项的 `cjList` 解析课程。
- 空字段给默认值。

统计方法：

```cpp
double gpa() const;
double requiredGpa() const;
double earnedCredits() const;
double weightedAvgScore() const;
double requiredWeightedAvgScore() const;
double creditsByAttribute(QString attr) const;
QList<TermGradeGroup> groupedByTerm() const;
```

GPA 规则：

- 仅统计 `passed && hasEffectiveScore` 的课程。
- 学分必须大于 0。
- `sum(gradePointScore * credit) / sum(credit)`。

加权均分规则：

- 仅统计 `passed && hasEffectiveScore` 的课程。
- `sum(courseScore * credit) / sum(credit)`。

必修统计：

- 仅统计 `courseAttributeName == "必修"`。

### 7.5 PassingScoreResult

对应及格成绩。

```cpp
struct PassingScoreGroup {
    QString label; // cjlx，如 2024-2025学年春(两学期)
    QList<GradeCourseItem> items;
};

struct PassingScoreResult {
    QList<PassingScoreGroup> groups;
};
```

解析：

- 根 JSON 的 `lnList` 每项转成 `PassingScoreGroup`。
- 每组的 `cjList` 转成课程。

排序复刻 Flutter：

1. 按 label 前 9 位学年倒序。
2. 同学年内春季排秋季前面。

统计方法：

```cpp
double gpa() const;
double totalCredits() const;
int totalPassed() const;
double weightedAvgScore() const;
double requiredWeightedAvgScore() const;
```

### 7.6 自定义统计

自定义统计基于方案成绩。

交互：

- 展示课程列表。
- 支持搜索课程名。
- 支持按课程属性筛选：全部、必修、选修、任选。
- 支持选择/取消选择单门课。
- 支持当前筛选范围全选/取消全选。
- 支持按学期全选/取消全选。
- 显示已选课程数。

统计：

- 已选课程 GPA。
- 已选课程学分。
- 已选课程加权均分。
- 已选课程必修均分。
- 已选课程通过/未通过数量。

课程唯一 key 复刻 Flutter：

```text
courseName|academicYearCode|termName|courseAttributeName
```

注意：

- 自定义统计不需要单独请求接口。
- 没有方案成绩缓存时，提示先获取方案成绩。

### 7.7 GradesViewModel

属性：

```cpp
Q_PROPERTY(QueryState schemeState READ schemeState NOTIFY schemeChanged)
Q_PROPERTY(QueryState passingState READ passingState NOTIFY passingChanged)
Q_PROPERTY(QString schemeErrorMessage READ schemeErrorMessage NOTIFY schemeChanged)
Q_PROPERTY(QString passingErrorMessage READ passingErrorMessage NOTIFY passingChanged)
Q_PROPERTY(QVariantMap schemeSummary READ schemeSummary NOTIFY schemeChanged)
Q_PROPERTY(QVariantMap passingSummary READ passingSummary NOTIFY passingChanged)
Q_PROPERTY(QVariantList schemeGroups READ schemeGroups NOTIFY schemeChanged)
Q_PROPERTY(QVariantList passingGroups READ passingGroups NOTIFY passingChanged)
Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)
```

方法：

```cpp
Q_INVOKABLE void load();
Q_INVOKABLE void refreshSchemeScores();
Q_INVOKABLE void refreshPassingScores();
Q_INVOKABLE void clearSchemeError();
Q_INVOKABLE void clearPassingError();
Q_INVOKABLE void setSearchQuery(QString query);
Q_INVOKABLE QVariantList filteredSchemeGroups() const;
Q_INVOKABLE QVariantList filteredPassingGroups() const;
Q_INVOKABLE QVariantMap customStatsForSelected(QVariantList selectedKeys) const;
```

加载流程：

1. 检查登录状态。
2. 未登录且无缓存：LoginRequired。
3. 读取方案成绩缓存。
4. 读取及格成绩缓存。
5. 有缓存则先显示。
6. 用户点击刷新时调用 B 的 API。
7. 成功后解析模型并写缓存。
8. 失败且有缓存：保留 Loaded，记录 error 用于 Toast。
9. 失败且无缓存：Error 或 LoginRequired。

### 7.8 页面细节

方案成绩 Tab：

- 顶部 summary panel：
  - GPA
  - 必修 GPA
  - 通过门数
  - 未通过门数
  - 平均分
  - 必修平均分
  - 已修学分
  - 要求学分
  - 选修学分
  - 任选学分
- 下方按学年学期分组。
- 每张课程卡显示：
  - 课程名
  - 英文名，若有
  - 课程属性
  - 学分
  - 原始成绩
  - 绩点
  - 等级
- 未通过课程用错误色弱提示。

及格成绩 Tab：

- 顶部 summary panel：
  - 总 GPA
  - 累计学分
  - 总通过门数
  - 学期数
  - 平均分
  - 必修平均分
- 下方按学期分组。
- 分组标题显示该学期通过门数和学分。

自定义统计 Tab：

- 顶部筛选 chip。
- 全选/取消全选按钮。
- 统计卡片。
- 按学期分组的可选课程列表。

## 8. 与其他人员的接口

### 8.1 D 与 A

D 使用：

- LoadingView。
- EmptyView。
- ErrorView。
- LoginRequiredView。
- Toast。
- Dialog。
- SearchBar。
- DataTable 或等价列表组件。

D 交付：

- `AcademicCalendarPage.qml`
- `ExamPlanPage.qml`
- `GradesPage.qml`
- ViewModel 注册名和入口组件名。

### 8.2 D 与 B

D 调用：

- `ZhjwApiService::fetchExamPlan()`
- `ZhjwApiService::fetchSchemeScores()`
- `ZhjwApiService::fetchPassingScores()`
- Auth 状态只读接口。

D 不调用：

- CookieHttpClient。
- ScuAuthService.login。
- ZhjwAuthService.getClient。

### 8.3 D 与 C

本阶段无直接依赖。

不要因为考表或成绩包含课程名就引用 C 的 `Course`。

### 8.4 D 与 E

D 提供：

- 校历 HTML fixture。
- 校历详情 HTML fixture。
- 考表 DTO fixture。
- 方案成绩 JSON fixture。
- 及格成绩 JSON fixture。
- ViewModel 可注入 fake API。

E 提供：

- Qt Test 框架。
- 手工验收脚本。
- 打包时网络权限检查。

## 9. 单元测试要求

D 至少覆盖：

校历：

- 列表页解析多个年份。
- 详情页解析多个图片 URL。
- 无条目时返回空。
- 非 UTF-8 页面解码 fallback。

考表：

- `isPast` 正常时间。
- `isPast` 时间解析失败 fallback。
- 排序规则。
- 空考表状态。
- 缓存读取和刷新失败保留旧数据。

成绩：

- `GradeCourseItem` 字段映射。
- `passed` 和 `hasEffectiveScore` 判断。
- 方案成绩 JSON 解析。
- 及格成绩 JSON 解析。
- GPA 计算。
- 必修 GPA 计算。
- 加权均分计算。
- 按学期分组排序。
- 搜索过滤。
- 课程属性筛选。
- 自定义统计。
- 缓存读取和刷新失败保留旧数据。

ViewModel：

- 未登录进入考表显示 LoginRequired。
- 未登录进入成绩显示 LoginRequired。
- 校历未登录仍可加载。

## 10. 开发步骤

### 阶段 D1：通用查询状态和缓存

1. 定义 QueryState。
2. 实现 QueryCacheRepository。
3. 实现 LastUpdatedLabel 和 RefreshButton。
4. 写缓存单测。

### 阶段 D2：校历

1. 实现 AcademicCalendarModels。
2. 实现 AcademicCalendarService。
3. 实现 AcademicCalendarViewModel。
4. 实现 AcademicCalendarPage。
5. 实现图片查看器。
6. 写解析测试。

### 阶段 D3：考表

1. 定义 ExamPlanItem。
2. 实现 ExamPlanViewModel。
3. 接入 B 的 fetchExamPlan。
4. 实现 ExamPlanPage 和 ExamCard。
5. 写 isPast 和排序测试。

### 阶段 D4：教务成绩

1. 定义 GradeCourseItem、SchemeScoreSummary、PassingScoreResult。
2. 实现 GradeStatisticsService。
3. 实现 GradesViewModel。
4. 接入 B 的 fetchSchemeScores。
5. 接入 B 的 fetchPassingScores。
6. 实现 SchemeScoresTab。
7. 实现 PassingScoresTab。
8. 实现 CustomStatsTab。
9. 写成绩解析和统计测试。

### 阶段 D5：联调与细化

1. 与 A 检查三页在 MainShell 中显示。
2. 与 B 检查未登录、登录、session 过期。
3. 与 E 跑手工验收。
4. 修复缓存和错误态细节。

## 11. 验收标准

人员 D 完成时必须满足：

- 校历页未登录可打开。
- 校历页可列出学年。
- 校历页可展示校历图片。
- 校历图片点击可放大查看。
- 考表页未登录显示登录提示。
- 考表页登录后可刷新。
- 考表为空时显示空状态。
- 考表按时间排序。
- 成绩页未登录显示登录提示。
- 成绩页登录后显示方案成绩、及格成绩、自定义统计三个 Tab。
- 方案成绩 summary 指标正确。
- 及格成绩 summary 指标正确。
- 自定义统计可以选择课程并计算 GPA/学分/均分。
- 搜索课程名可用。
- 查询结果有上次更新时间。
- 刷新失败不清空已有缓存。
- D 的页面不直接写认证、Cookie、教务 SSO 逻辑。
- 不实现体测查询。

## 12. 文档间冲突检查结果

人员 D 与其他人员边界：

- D 拥有校历、考表、教务成绩页面和 ViewModel。
- B 拥有教务 API、认证。
- A 拥有公共 UI 和路由壳。
- C 拥有课表，不参与成绩模型。
- E 拥有测试和打包。

明确避免的冲突：

- 校历抓取由 D 实现，因为它是公开页面，不需要 B 的认证链。
- 考表 HTML 解析由 B 的 API service 完成，D 不重复解析教务 HTML。
- 成绩 callback URL 提取和请求由 B 完成，D 负责成绩业务模型和统计。
- QueryCacheRepository 只缓存查询模块，不缓存课表数据。
- 体测查询放入后续可选项。

需要持续协调的点：

- B 修改 Exam/Grades API 返回策略时通知 D。
- A 修改公共状态组件属性时 D 同步 QML。
- E 新增 fixture 时 D 保持模型字段兼容。
