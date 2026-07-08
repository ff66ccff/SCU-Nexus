# 人员 C：课表管理详细任务书

## 0. 本文档定位

人员 C 负责第一阶段的“课表管理”完整闭环：

- 课表领域模型
- 本地 SQLite 存储
- 课表 ViewModel
- 课表周视图
- 课程新增、编辑、删除
- 多课表管理
- 从教务系统导入课表
- 当前教学周同步

课表模块必须能离线查看和编辑本地课表；只有“从教务系统导入”和“同步当前周”需要人员 B 的认证/API。

## 1. 迁移依据

主要参考 Flutter 项目：

- `Bugaoshan/lib/models/course.dart`
- `Bugaoshan/lib/providers/course_provider.dart`
- `Bugaoshan/lib/services/database_service.dart`
- `Bugaoshan/lib/pages/course/course_page.dart`
- `Bugaoshan/lib/pages/course/course_edit_page.dart`
- `Bugaoshan/lib/pages/course/import_schedule_page.dart`
- `Bugaoshan/lib/pages/course/schedule_management_page.dart`
- `Bugaoshan/lib/pages/course/course_schedule_setting.dart`
- `Bugaoshan/lib/pages/course/time_slot_setting_page.dart`
- `Bugaoshan/lib/widgets/course/course_grid.dart`
- `Bugaoshan/lib/widgets/course/grid_day_column.dart`
- `Bugaoshan/lib/widgets/course/grid_logic.dart`
- `Bugaoshan/test/widget_test.dart`

迁移原则：

- 领域逻辑尽量复刻 Flutter。
- UI 适配桌面，不追求移动端滑动体验。
- 本地数据结构与 Flutter 版保持兼容方向，方便未来导入/导出。
- QML 只做显示和交互，计算逻辑放在 C++ Model/ViewModel。

## 2. 推荐文件归属

人员 C 拥有：

```text
SCU_Nexus/src/
  models/
    Course.h
    Course.cpp
    ScheduleConfig.h
    ScheduleConfig.cpp
    TimeSlot.h
    WeekType.h
  repositories/
    ScheduleRepository.h
    ScheduleRepository.cpp
  services/
    course/
      JwxtScheduleParser.h
      JwxtScheduleParser.cpp
      ScheduleImportService.h
      ScheduleImportService.cpp
      CourseLayoutService.h
      CourseLayoutService.cpp
  viewmodels/
    ScheduleViewModel.h
    ScheduleViewModel.cpp
    ScheduleImportViewModel.h
    ScheduleImportViewModel.cpp
    CourseEditViewModel.h
    CourseEditViewModel.cpp
  qml/
    pages/
      schedule/
        SchedulePage.qml
        CourseEditDialog.qml
        ImportScheduleDialog.qml
        ScheduleManagementDialog.qml
        ScheduleSettingsDialog.qml
        TimeSlotSettingsDialog.qml
    components/
      schedule/
        CourseGrid.qml
        CourseCard.qml
        WeekSwitcher.qml
        DayColumn.qml
        SectionColumn.qml
        CourseConflictBadge.qml
```

人员 C 不拥有：

- 登录页。
- 教务登录/SSO。
- HTTP 客户端。
- 考表、校历、成绩页面。
- Windows 打包脚本。

## 3. 数据模型

### 3.1 WeekType

对应 Flutter：

```dart
enum WeekType { every, odd, even }
```

Qt 版：

```cpp
enum class WeekType {
    Every = 0,
    Odd = 1,
    Even = 2
};
```

数据库中保存枚举值：

```text
0 = Every
1 = Odd
2 = Even
```

### 3.2 TimeSlot

字段：

```cpp
struct TimeSlot {
    QTime startTime;
    QTime endTime;
};
```

必须支持：

- `toJson()`
- `fromJson()`
- 校验 `startTime < endTime`

JSON 兼容 Flutter：

```json
{
  "startTime": { "hour": 8, "minute": 15 },
  "endTime": { "hour": 9, "minute": 0 }
}
```

### 3.3 ScheduleConfig

字段：

```cpp
class ScheduleConfig {
public:
    QString id;
    QString semesterName;
    QDate semesterStartDate;
    int totalWeeks = 20;
    int morningSections = 4;
    int afternoonSections = 5;
    int eveningSections = 3;
    int courseDuration = 45;
    int breakDuration = 10;
    bool autoSyncTime = true;
    QVector<TimeSlot> timeSlots;
};
```

派生属性：

```cpp
int sectionsPerDay() const;
int currentWeek(QDate today = QDate::currentDate()) const;
```

`currentWeek()` 规则复刻 Flutter：

1. 把 `semesterStartDate` 和 today 都按日期比较。
2. 如果 today 早于开学日，返回 1。
3. `floor(days / 7) + 1`。
4. clamp 到 `[1, totalWeeks]`。

### 3.4 默认时间表

必须迁移 Flutter 版两个预设。

江安校区 4-5-3：

```text
1  08:15-09:00
2  09:10-09:55
3  10:15-11:00
4  11:10-11:55
5  13:50-14:35
6  14:45-15:30
7  15:40-16:25
8  16:45-17:30
9  17:40-18:25
10 19:20-20:05
11 20:15-21:00
12 21:10-21:55
```

望江/华西 4-5-3：

```text
1  08:00-08:45
2  08:55-09:40
3  10:00-10:45
4  10:55-11:40
5  14:00-14:45
6  14:55-15:40
7  15:50-16:35
8  16:55-17:40
9  17:50-18:35
10 19:30-20:15
11 20:25-21:10
12 21:20-22:05
```

方法：

```cpp
static QVector<TimeSlot> jiangAnTimeSlots();
static QVector<TimeSlot> wangJiangHuaXiTimeSlots();
static std::optional<QVector<TimeSlot>> timeSlotsForCampusName(QString campusName);
```

匹配规则：

- 包含 `江安` 返回江安。
- 包含 `望江` 或 `华西` 返回望江/华西。
- 其他返回空。

### 3.5 Course

字段：

```cpp
class Course {
public:
    QString id;
    QString name;
    QString teacher;
    QString location;
    int startWeek;
    int endWeek;
    int dayOfWeek;      // 1=Mon ... 7=Sun
    int startSection;
    int endSection;
    quint32 colorValue; // ARGB
    WeekType weekType;
};
```

必须实现：

```cpp
bool isInWeekRange(int week) const;
bool isActiveInWeek(int week) const;
bool conflictsWith(const Course& other, const QString& excludeId = {}) const;
QJsonObject toJson() const;
static Course fromJson(const QJsonObject& json);
Course copyWith(...) const;
```

`isActiveInWeek()` 规则：

- 不在起止周内：false。
- 单周课且 week 为偶数：false。
- 双周课且 week 为奇数：false。
- 其他 true。

`conflictsWith()` 规则：

1. `excludeId` 等于自身 id 时 false。
2. 星期不同 false。
3. 节次区间不重叠 false。
4. 周次区间不重叠 false。
5. 单双周互斥时 false。
6. 其他 true。

单双周共享周判断必须复刻 Flutter `_hasSharedWeek()`，不要只判断区间重叠。

## 4. SQLite Repository

### 4.1 数据库位置

使用 Qt 标准路径：

```cpp
QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
```

数据库文件建议：

```text
scu_nexus.db
```

### 4.2 表结构

第一阶段使用 Flutter 同构结构：

```sql
CREATE TABLE metadata (
  key TEXT PRIMARY KEY,
  value TEXT NOT NULL
);

CREATE TABLE schedules (
  id TEXT PRIMARY KEY,
  config_json TEXT NOT NULL
);

CREATE TABLE courses (
  id TEXT PRIMARY KEY,
  schedule_id TEXT NOT NULL,
  name TEXT,
  teacher TEXT,
  location TEXT,
  start_week INTEGER,
  end_week INTEGER,
  day_of_week INTEGER,
  start_section INTEGER,
  end_section INTEGER,
  color_value INTEGER,
  week_type INTEGER,
  FOREIGN KEY (schedule_id) REFERENCES schedules(id) ON DELETE CASCADE
);
```

关键 metadata：

```text
currentScheduleId
```

### 4.3 初始化规则

复刻 Flutter 新逻辑：

- 新安装时不自动创建默认课表。
- `schedules` 为空时，`currentScheduleId` 为空字符串。
- ViewModel 使用占位配置保护 UI 计算。
- 用户导入或新建后才产生真实课表。

### 4.4 Repository 接口

```cpp
class ScheduleRepository : public QObject {
public:
    bool init();

    QString currentScheduleId() const;
    QList<ScheduleConfig> allSchedules() const;
    ScheduleConfig currentScheduleConfig() const;
    QList<Course> currentCourses() const;

    bool switchSchedule(const QString& scheduleId);
    bool addSchedule(const ScheduleConfig& config);
    bool deleteSchedule(const QString& scheduleId);
    bool saveScheduleConfig(const ScheduleConfig& config);

    QList<Course> coursesForSchedule(const QString& scheduleId) const;
    bool addCourse(const Course& course);
    bool updateCourse(const Course& course);
    bool deleteCourse(const QString& courseId);
    bool replaceScheduleCourses(const QString& scheduleId, const QList<Course>& courses);
    bool hasConflict(const Course& course, const QString& excludeId = {}) const;

    bool clearAllCourseData();
};
```

### 4.5 缓存规则

与 Flutter 一样维护内存缓存：

- `_currentScheduleId`
- `_schedulesCache`
- `_coursesCache`

写操作后必须刷新缓存。

`getCourses(scheduleId)` 如果不是当前课表：

- Qt 版可以直接查库，不必复刻 Flutter 同步方法返回空的限制。
- 但公开接口必须标明是否访问当前缓存或数据库。

### 4.6 删除课表规则

删除当前课表：

- 如果还有其他课表，切到第一张。
- 如果没有课表，清空 `currentScheduleId`，课程缓存为空。

删除非当前课表：

- 不切换当前课表。
- 刷新 allSchedules。

## 5. ScheduleViewModel

### 5.1 QML 属性

```cpp
Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
Q_PROPERTY(bool hasSchedule READ hasSchedule NOTIFY schedulesChanged)
Q_PROPERTY(int currentWeek READ currentWeek NOTIFY currentWeekChanged)
Q_PROPERTY(int totalWeeks READ totalWeeks NOTIFY configChanged)
Q_PROPERTY(QString currentScheduleName READ currentScheduleName NOTIFY configChanged)
Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)
```

课程列表建议用 `QAbstractListModel`：

角色：

```text
id
name
teacher
location
startWeek
endWeek
dayOfWeek
startSection
endSection
colorValue
weekType
active
conflict
track
totalTracks
```

### 5.2 QML 方法

```cpp
Q_INVOKABLE void load();
Q_INVOKABLE void switchSchedule(QString scheduleId);
Q_INVOKABLE void updateCurrentWeek(int week);
Q_INVOKABLE void goPreviousWeek();
Q_INVOKABLE void goNextWeek();
Q_INVOKABLE void addCourse(QVariantMap data);
Q_INVOKABLE void updateCourse(QString id, QVariantMap data);
Q_INVOKABLE void deleteCourse(QString id);
Q_INVOKABLE void clearAllCourseData();
Q_INVOKABLE QVariantList schedules() const;
Q_INVOKABLE QVariantMap courseById(QString id) const;
```

### 5.3 状态刷新规则

任何改变课程或配置的操作后：

1. Repository 写入。
2. ViewModel 重新加载当前课程。
3. 重新计算当前周可见课程。
4. 发出对应 `dataChanged` 或 `modelReset`。
5. 通知 A 的设置页清缓存状态更新。

## 6. 教务课表导入

### 6.1 B 提供的输入

人员 B 的接口：

```cpp
QFuture<QList<SemesterDto>> ZhjwApiService::fetchSemesters();
QFuture<QJsonObject> ZhjwApiService::fetchJwxtSchedule(QString planCode);
QFuture<int> ZhjwApiService::fetchCurrentWeek();
```

C 不拼 URL，不处理 Cookie。

### 6.2 学期标签清理

复刻 Flutter：

```text
"（当前）" -> ""
"(当前)" -> ""
trim
```

方法：

```cpp
QString cleanSemesterLabel(QString label);
```

### 6.3 JwxtScheduleParser

输入：

```json
{
  "xkxx": [...]
}
```

遍历规则复刻 Flutter：

1. `xkxx` 是数组。
2. 数组每项是课程 map。
3. 遍历每个 key/value。
4. value 是课程详情。
5. 课程原名：`courseName`。
6. 课程序号：`id.coureSequenceNumber`。
7. 显示名：`courseName + " (" + coureSequenceNumber + ")"`。
8. 教师：`attendClassTeacher`。
9. 时间地点：`timeAndPlaceList`。

每个 `timeAndPlaceList` 元素生成一个 Course：

```text
dayOfWeek = classDay
startSection = classSessions
continuingSession = continuingSession
endSection = startSection + continuingSession - 1
location = teachingBuildingName + classroomName
classWeek = bit string
```

### 6.4 classWeek 解析

`classWeek` 是 bit string，例如：

```text
111111111111111100000000
```

规则：

1. 从左到右遍历。
2. 第 i 位为 `1` 表示第 `i + 1` 周上课。
3. 第一位 1 是 `startWeek`。
4. 最后一位 1 是 `endWeek`。
5. activeWeeks 全是奇数则 `WeekType::Odd`。
6. activeWeeks 全是偶数则 `WeekType::Even`。
7. 否则 `WeekType::Every`。
8. 如果没有任何 1，跳过该时间地点。

注意：

- 该表示会丢失“非连续但非单双周”的精确信息。Flutter 版也是用 start/end + WeekType 表示，Qt 第一阶段保持一致。
- 后续如需精确周次，可另增 `weekMask` 字段，但第一阶段不要破坏当前模型。

### 6.5 课程颜色

复刻 Flutter 用颜色池轮询。Qt 版建议：

```text
0xFFEF5350
0xFFEC407A
0xFFAB47BC
0xFF7E57C2
0xFF5C6BC0
0xFF42A5F5
0xFF26C6DA
0xFF26A69A
0xFF66BB6A
0xFF9CCC65
0xFFFFA726
0xFF8D6E63
```

颜色分配以生成课程顺序递增。

### 6.6 导入配置

导入生成的 ScheduleConfig：

- `semesterStartDate = 当前周一`
- `semesterName = 清理后的学期名`
- `totalWeeks = 20`
- `morningSections = 4`
- `afternoonSections = 5`
- `eveningSections = 3`
- `timeSlots = 江安默认时间表`

导入成功后允许用户用 `fetchCurrentWeek()` 自动校准开学日期：

1. B 返回当前教学周 week。
2. 取今天。
3. 转为本周周日。
4. `newStartDate = currentSunday - (week - 1) * 7 days`。
5. 更新当前课表配置。

注意：这里复刻 Flutter 的 `toSunday()` 规则。

### 6.7 导入校验

必须检查：

- `totalWeeks >= 1`
- `timeSlots` 非空
- `course.startWeek >= 1`
- `course.endWeek >= course.startWeek`
- `course.endWeek <= totalWeeks`
- `dayOfWeek` 在 1 到 7
- `startSection >= 1`
- `endSection >= startSection`
- `endSection <= timeSlots.length`
- 课程名非空，若为空设为 `Unknown` 或阻止导入

校验失败时：

- 不写入任何部分数据。
- 提示“导入数据格式异常”。
- 日志记录具体课程。

### 6.8 名称冲突处理

单个导入：

1. 如果无冲突，直接添加。
2. 如果同名，弹窗给三个选择：
   - 取消
   - 添加后缀
   - 更新已有课表

批量导入：

1. 先检查是否存在任何冲突。
2. 如果有冲突，询问统一策略：
   - 取消
   - 全部添加后缀
   - 全部更新已有课表
3. 无冲突则按服务端顺序导入。

更新已有课表：

- 调用 `replaceScheduleCourses(scheduleId, courses)`。
- 不改变原课表配置，除非用户明确选择更新配置。
- 新课程必须重新生成 id，避免主键冲突。

### 6.9 导入当前学期切换

如果学期 label 包含 `（当前）` 或 `(当前)`：

- 记录清理后的名称。
- 批量导入结束后切换到该课表。

## 7. 课表页面

### 7.1 页面结构

`SchedulePage.qml`：

- 顶部：课表名、当前周、上一周/下一周、回到本周。
- 操作区：导入课表、新建课程、课表管理、设置。
- 主体：周视图网格。
- 空状态：无课表时提示导入或新建。

### 7.2 周视图

横向：

- 周一到周日。
- 如果没有周末课程，可以提供隐藏周末选项；第一阶段默认显示 7 天，避免逻辑复杂。

纵向：

- 1 到 `sectionsPerDay()`。
- 每节高度固定，建议 60-72 px。
- 左侧显示节次和时间。

课程卡片：

- 显示课程名。
- 显示地点。
- 显示教师。
- 非当前周课程如果显示，应置灰。
- 冲突课程显示角标或并排轨道。

### 7.3 可见课程筛选

复刻 `selectVisibleCoursesForDay()`：

输入：

- 全部课程
- displayWeek
- showNonCurrentWeekCourses

规则：

- `showNonCurrentWeekCourses = false` 时只显示 `isActiveInWeek(displayWeek)`。
- `true` 时：
  - 先取在周次范围内的课程。
  - 再加入未来周次开始、且与当前可见课程节次不冲突的课程。
  - 按布局排序。

布局排序：

1. 开始节次升序。
2. 持续时间降序。
3. 开始周升序。

### 7.4 轨道分配

复刻 `assignCourseTracks()`：

- 同一天同节次重叠课程并排显示。
- track 从 0 开始。
- totalTracks 是当前课程实际重叠范围内的课程数。
- 不能因为全天最大冲突数导致所有卡片都被压窄。

### 7.5 同槽课程合并

复刻 `mergeSameSlotCourses()`：

合并条件：

- 课程名相同。
- 星期相同。
- 起止节次相同。
- 位置相同。

合并结果：

- 教师去重，用 `、` 连接。
- 地点去重。
- 周次取最小 startWeek 和最大 endWeek。
- 如果 weekType 都一样，保留；否则 Every。

## 8. 课程编辑

### 8.1 字段

新增/编辑课程必须支持：

- 课程名
- 教师
- 地点
- 起始周
- 结束周
- 单双周
- 星期
- 起始节次
- 结束节次
- 颜色

### 8.2 默认值

新增：

- 名称空。
- 教师空。
- 地点空。
- startWeek = 1。
- endWeek = 当前课表 totalWeeks。
- dayOfWeek = 用户点击的列或周一。
- startSection = 用户点击的节次或 1。
- endSection = startSection + 1，不能超过最大节次。
- weekType = Every。
- 颜色从预设中随机或顺序选择。

编辑：

- 完全读取原课程。

### 8.3 保存校验

必须校验：

- 课程名不能为空。
- 起始周不能大于结束周。
- 起始节不能大于结束节。
- 星期必须在 1 到 7。
- 节次不能超过当前配置。
- 起止节次不能跨上午/下午/晚上。

跨时段规则：

```text
morning:   1 .. morningSections
afternoon: morningSections+1 .. morningSections+afternoonSections
evening:   afternoonEnd+1 .. sectionsPerDay
```

若跨段，提示：

```text
一门课不能跨越上午、下午或晚上时段。
```

### 8.4 冲突处理

Flutter 当前是检测冲突后直接提示并阻止保存。原分工里写“弹窗确认”，二者有差异。

本阶段统一采用更安全规则：

- 默认阻止保存。
- 弹窗展示冲突课程名称和时间。
- 如果团队想允许强制保存，应另增“仍然保存”按钮，并由产品确认。

为了减少数据混乱，推荐第一阶段不允许强制保存。

## 9. 多课表管理

必须支持：

- 查看全部课表。
- 切换当前课表。
- 新建空课表。
- 重命名课表。
- 删除课表。
- 复制课表可作为增强项，不阻塞第一阶段。

删除规则：

- 删除前确认。
- 删除当前课表后按 Repository 规则切换。
- 删除最后一张后显示无课表 EmptyView。

新建空课表：

- 用户输入名称。
- 自动生成 id。
- 默认配置使用江安时间表。
- 创建后切换到该课表。

## 10. 课表设置

第一阶段设置项：

- 学期名称。
- 开学日期。
- 总周数。
- 上午/下午/晚上节数。
- 课程时长。
- 课间时长。
- 时间表预设：江安、望江/华西、自定义。

保存后：

- 如果节次数减少导致课程越界，应阻止保存并提示越界课程。
- 如果只是时间变化，不影响课程节次。
- 更新后刷新周视图。

## 11. 与其他人员的接口

### 11.1 C 与 A

C 交付：

- `SchedulePage.qml`
- `ScheduleViewModel`
- `clearAllCourseData()`

A 提供：

- MainShell 路由。
- LoadingView/EmptyView/ErrorView。
- Dialog/Toast 公共组件。

C 不自己实现全局导航壳。

### 11.2 C 与 B

C 调用：

```cpp
fetchSemesters()
fetchJwxtSchedule(planCode)
fetchCurrentWeek()
```

C 不调用：

- CookieHttpClient。
- ScuAuthService。
- ZhjwAuthService。

### 11.3 C 与 D

本阶段无直接依赖。

潜在共享：

- 当前教学周概念。
- 日期工具。

如果需要共享日期工具，应放到 `core/date`，不要互相 include 页面类。

### 11.4 C 与 E

E 提供测试框架和测试夹具。

C 提供：

- 纯 C++ Course/ScheduleConfig 测试入口。
- JwxtScheduleParser 的 sample JSON。
- Repository 可注入临时数据库路径。

## 12. 单元测试要求

必须覆盖：

- `Course::isInWeekRange`
- `Course::isActiveInWeek`
- `Course::conflictsWith`
- 单双周冲突判断
- 节次重叠判断
- `ScheduleConfig::currentWeek`
- 江安时间表长度和关键时间点
- 望江/华西时间表长度和关键时间点
- `timeSlotsForCampusName`
- Repository 初始化空数据库
- add/switch/delete schedule
- add/update/delete course
- replaceScheduleCourses
- clearAllCourseData
- Jwxt classWeek 连续周解析
- Jwxt 单周解析
- Jwxt 双周解析
- Jwxt 空周次跳过
- 导入名称冲突策略

## 13. 开发步骤

### 阶段 C1：纯模型

1. 实现 WeekType。
2. 实现 TimeSlot。
3. 实现 ScheduleConfig。
4. 实现 Course。
5. 写模型单测。

### 阶段 C2：Repository

1. 建库。
2. 建三张表。
3. 实现缓存。
4. 实现课表增删改查。
5. 实现课程增删改查。
6. 写数据库单测。

### 阶段 C3：ScheduleViewModel

1. 绑定 Repository。
2. 暴露 QML 属性。
3. 暴露课程 list model。
4. 实现周切换。
5. 实现清缓存。

### 阶段 C4：周视图 UI

1. 实现空状态。
2. 实现周切换条。
3. 实现节次列。
4. 实现日列。
5. 实现课程卡片。
6. 实现冲突并排。
7. 接入 A 的公共组件。

### 阶段 C5：编辑和管理

1. 新增课程弹窗。
2. 编辑课程弹窗。
3. 删除课程确认。
4. 课表管理弹窗。
5. 课表设置弹窗。

### 阶段 C6：在线导入

1. 实现 JwxtScheduleParser。
2. 实现导入 ViewModel。
3. 接入 B 的学期列表。
4. 接入 B 的课表 JSON。
5. 实现名称冲突处理。
6. 实现当前周同步。
7. 完成联调。

## 14. 验收标准

人员 C 完成时必须满足：

- 无课表时显示空状态。
- 可以新建空课表。
- 可以新增课程。
- 可以编辑课程。
- 可以删除课程。
- 可以切换周次。
- 可以切换课表。
- 可以删除课表。
- 可以从教务系统导入单个学期。
- 可以处理同名课表冲突。
- 可以同步当前教学周。
- 重启应用后课表仍存在。
- 离线状态下可以查看和编辑已保存课表。
- 课表模块不直接写 HTTP。
- 课表模块不处理账号密码或 Cookie。

## 15. 文档间冲突检查结果

人员 C 与其他人员边界：

- C 拥有课表领域模型、Repository、课表页面。
- B 拥有远端 API，不保存课表。
- A 拥有公共 UI，不写课表业务逻辑。
- D 不修改课表模型。
- E 不修改业务实现，只写测试、打包和验收。

明确避免的冲突：

- `Course` 和 `ScheduleConfig` 只由 C 定义，其他模块使用时 include，不重复建 DTO。
- `fetchJwxtSchedule()` 的 JSON 解析为课程只由 C 做。
- 当前周同步的 API 调用由 B 提供，开学日期计算和配置保存由 C 做。
- 设置页“清除课表缓存”调用 C，不由 A 直接删数据库。

需要持续协调的点：

- C 修改 Course 字段时通知 E 更新测试，通知 A/D 确认是否有显示依赖。
- C 修改数据库 schema 时必须写迁移说明。
- C 修改 QML 页面入口名时通知 A 更新路由。
