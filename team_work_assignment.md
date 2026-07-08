# Bugaoshan Qt Windows 移植实现细节

本文档描述每个人在具体功能上应该如何实现。它比总览文档更偏代码结构、接口设计和页面行为。

## 通用工程约定

### 推荐目录结构

```text
qt-client/
  CMakeLists.txt
  src/
    main.cpp
    app/
      AppController.*
      Router.*
      AppSettings.*
    core/
      network/
      storage/
      crypto/
      logging/
    services/
      auth/
      api/
      course/
    models/
    repositories/
    viewmodels/
    qml/
      App.qml
      pages/
      components/
      styles/
    tests/
```

### 分层规则

- QML 只负责界面和交互，不直接写 HTTP 逻辑。
- ViewModel 暴露给 QML，负责状态、错误、命令方法。
- Service 负责业务流程。
- Repository 负责本地数据库。
- API Service 负责远端请求和解析。
- Model 尽量保持普通 C++ 数据结构，便于测试。

## 人员 A：应用框架和公共 UI

### 1. Qt 工程骨架

要做的事：

- 创建 Qt 6 CMake 工程。
- 配置 C++17 或 C++20。
- 引入 Qt Quick、Qt Quick Controls、Qt Network、Qt SQL。
- 如果使用 Qt WebEngine 预留 CMake 开关，但第一阶段不强依赖。

建议模块：

```text
AppController
Router
AppSettings
ToastManager
DialogManager
ThemeManager
```

### 2. 主界面

页面结构建议：

- 登录前：LoginPage。
- 登录后：MainShell。
- MainShell 左侧导航包含：
  - 课表
  - 校历
  - 考表
  - 成绩
  - 设置

实现要点：

- 主窗口默认 1100x760，最小 900x620。
- 记住窗口大小和位置。
- 页面切换不要重新创建全局服务。
- 登录状态失效时跳回 LoginPage，并保留清晰提示。

### 3. 公共组件

至少实现：

- LoadingView
- EmptyView
- ErrorView
- SearchBar
- PrimaryButton
- SecondaryButton
- FormField
- DataTable
- SectionHeader
- ConfirmDialog
- Toast

每个查询页必须统一使用 Loading、Empty、Error 三种状态。

### 4. 设置页

第一阶段设置页只需要：

- 账号退出登录。
- 清除本地缓存。
- 主题跟随系统或浅色/深色切换。
- 查看版本号。

## 人员 B：认证、网络和 API

### 1. 删除 OCR 流程

Flutter 版当前有 `OcrService` 和 `assets/universal-login-ocr.tflite`。Qt 版不实现该服务。

登录流程改为：

1. 请求验证码接口。
2. 将 base64 验证码图片显示在 LoginPage。
3. 用户手动输入验证码文本。
4. 提交账号、密码、验证码 code、验证码文本。

建议接口：

```cpp
struct CaptchaResult {
    QString code;
    QByteArray imageBytes;
};

class AuthService : public QObject {
    Q_OBJECT
public:
    Q_INVOKABLE void fetchCaptcha();
    Q_INVOKABLE void login(QString username, QString password, QString captchaText);
signals:
    void captchaLoaded(QByteArray imageBytes);
    void loginSucceeded();
    void loginFailed(QString message);
};
```

### 2. Cookie HTTP 客户端

对应 Flutter 版 `CookieClient`。

必须实现：

- 按域名保存 Cookie。
- 请求时只发送当前域名或父域匹配的 Cookie。
- 手动处理 302 跳转。
- 每一跳都保存 Set-Cookie。
- 支持超时和错误重试。

建议类：

```cpp
class CookieHttpClient : public QObject {
public:
    QNetworkReply* get(const QUrl& url, const Headers& headers = {});
    QNetworkReply* post(const QUrl& url, const QByteArray& body, const Headers& headers = {});
    Task<HttpResponse> followRedirects(const QUrl& url, const Headers& headers = {}, int maxRedirects = 10);
};
```

### 3. 统一认证

对应 Flutter 版 `ScuAuth`。

要实现：

- 获取验证码。
- 获取 SM2 公钥。
- 使用 SM2 加密密码。
- 请求 access token。
- 保存 token 和登录时间。
- 应用启动时恢复 token。
- 1 小时过期判断。
- 绑定统一认证 session。
- 自动刷新失败后通知 UI 重新登录。

关键地址沿用 Flutter 版：

- `https://id.scu.edu.cn`
- `captcha`
- `sm2_key`
- `rest_token`

注意：

- `access_token` 建议进入安全存储，不要明文放普通配置文件。
- 如果安全存储第一阶段来不及做，可以临时使用 QSettings，但要标注为技术债。

### 4. 教务认证

对应 Flutter 版：

- `ZhjwAuth`
- `SsoRelayAuth`
- `kZhjwBase = http://zhjw.scu.edu.cn`

要实现：

- 通过统一认证拿到教务 Cookie。
- 缓存已登录的 CookieHttpClient。
- 会话过期时清理并重试一次。

### 5. 教务 API

第一阶段至少实现：

```text
fetchCurrentWeek()
fetchSemesters()
fetchJwxtSchedule(planCode)
fetchPassingScores()
fetchSchemeScores()
fetchExamPlan()
fetchAcademicCalendar()
```

其中 `fetchAcademicCalendar()` 如果现有 Flutter 版没有统一 API，可先按校历页面现有逻辑复刻；若校历当前是静态规则或页面解析，也保持同等实现。

错误类型建议：

```cpp
enum class ApiErrorType {
    Network,
    Unauthenticated,
    SessionExpired,
    ServiceUnavailable,
    ParseFailed,
    Unknown
};
```

## 人员 C：课表管理

### 1. 数据模型

从 Flutter 版迁移：

- TimeSlot
- ScheduleConfig
- Course
- WeekType

Qt 版不要直接依赖 QML 类型。建议模型使用 C++，ViewModel 再转换为 QVariantMap/QAbstractListModel。

建议字段：

```text
Course:
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
```

必须实现的方法：

- isActiveInWeek(week)
- conflictsWith(other)
- toJson()
- fromJson()

### 2. SQLite

表结构可沿用 Flutter 版：

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

Repository 方法：

```text
init()
getCurrentScheduleId()
getAllSchedules()
getScheduleConfig()
switchSchedule(scheduleId)
addSchedule(config)
deleteSchedule(scheduleId)
saveScheduleConfig(config)
getCourses(scheduleId)
addCourse(course)
updateCourse(course)
deleteCourse(courseId)
replaceScheduleCourses(scheduleId, courses)
clearAllCourseData()
```

### 3. 课表导入

页面流程：

1. 用户打开“导入课表”。
2. 请求学期列表 `fetchSemesters()`。
3. 用户选择学期。
4. 调用 `fetchJwxtSchedule(planCode)`。
5. 解析课程列表。
6. 用户确认导入。
7. 保存为一个新的 ScheduleConfig 和课程列表。

导入时必须处理：

- 同名课表已存在。
- 空课表。
- 课程节次不合法。
- 周次范围不合法。
- 网络失败。
- 会话过期。

### 4. 课表展示

第一阶段建议做周视图：

- 横向：周一到周日。
- 纵向：节次。
- 课程卡片显示课程名、地点、教师。
- 支持当前周切换。
- 非当前周课程不显示或置灰。
- 冲突课程用明显提示。

### 5. 课程编辑

字段：

- 课程名
- 教师
- 地点
- 起止周
- 单双周
- 星期
- 起止节次
- 颜色

保存前校验：

- 课程名不能为空。
- 起始周不能大于结束周。
- 起始节不能大于结束节。
- 星期必须在 1 到 7。
- 与已有课程冲突时弹窗确认。

## 人员 D：成绩、考表、校历

### 1. 成绩查询

数据来源：

- `fetchSchemeScores()`
- `fetchPassingScores()`

页面结构建议：

- 顶部显示总学分、平均绩点、课程数。
- Tab 1：方案成绩。
- Tab 2：及格成绩。
- Tab 3：统计。

ViewModel 状态：

```text
idle
loading
loaded
error
```

缓存策略：

- 成绩 JSON 原始数据缓存到本地。
- 打开页面先显示缓存，再允许用户刷新。
- 刷新失败时保留旧数据，并显示错误提示。

统计建议：

- 总学分。
- 通过学分。
- 按课程属性或类别分组。
- GPA 或平均分按现有 Flutter 模型逻辑复刻。

### 2. 考表查询

数据来源：

- `fetchExamPlan()`

页面结构：

- 顶部刷新按钮。
- 列表展示考试课程。
- 每项显示课程名、考试时间、地点、座位号、考试类型。

处理规则：

- 没有考试时显示空状态。
- 考试时间解析失败时显示原始文本。
- 支持按时间排序。
- 缓存最近一次结果。

### 3. 校历查询

第一阶段可以采用轻量实现：

- 显示当前学期周次。
- 显示学期起止日期。
- 显示节假日、调休、特殊日期。
- 支持按月份查看。

如果复刻 Flutter `HolidayUtils`：

- 使用公历节假日规则。
- 标记法定节假日。
- 标记调休工作日。
- 标记节气或节日可作为增强项，不阻塞第一阶段。

页面结构：

- 月历视图。
- 今日高亮。
- 当前教学周提示。
- 重要日期列表。

### 4. 查询类页面统一行为

三个页面都要支持：

- 首次加载 loading。
- 网络失败 error。
- 未登录或会话过期提示。
- 空数据 empty。
- 手动刷新。
- 显示上次更新时间。

## 人员 E：测试、打包和可选功能预研

### 1. 自动化测试

优先测试：

- Course::isActiveInWeek
- Course::conflictsWith
- ScheduleConfig::getCurrentWeek
- SQLite Repository 增删改查
- 成绩 JSON 解析
- 考表 JSON 解析
- Cookie 域名匹配
- 版本和配置读取

测试数据建议从 Flutter 测试中迁移：

- `exam_plan_test.dart`
- `calendar_event_utils_test.dart`
- `ics_service_test.dart` 中可复用的日期和课程样例

### 2. 手工验收清单

必须覆盖：

- 首次启动。
- 登录成功。
- 验证码错误。
- 密码错误。
- 断网登录。
- 会话过期后刷新成绩。
- 导入课表。
- 编辑课程。
- 删除课表。
- 查询成绩。
- 查询考表。
- 查看校历。
- 退出登录。
- 重启应用后本地数据仍在。

### 3. Windows 打包

建议流程：

1. Release 构建。
2. 运行 `windeployqt`。
3. 复制资源文件。
4. 复制 SQLite 驱动和必要 DLL。
5. 生成 zip 包或安装包。
6. 在干净环境验证启动。

包内必须包含：

- 应用 exe。
- Qt 运行库。
- QML 模块。
- 图片资源。
- 许可证文本。

### 4. 可选模块预研

第二阶段建议优先级：

1. 培养方案：与教务 API 同源，复用人员 B 的教务认证，优先级高。
2. 体测查询：SSO relay 类模块，功能相对单一，优先级中高。
3. 校园网设备查询：HTTP 查询类，优先级中。
4. 校园卡、网费查询：依赖微服务或缴费平台认证，优先级中。
5. 寝室电费、空调余额查询：依赖绑定信息和 PayApp，优先级中。
6. 第二课堂：独立 OAuth/token 和活动业务较多，工作量最大，建议最后做。

每个可选模块预研需输出：

- 是否需要新增认证方式。
- 需要复刻哪些 Flutter 文件。
- API 数量。
- 页面数量。
- 预计工作量。
- 最大风险。

## 第一阶段接口对接顺序

建议按以下顺序集成：

1. A 完成主窗口和登录页壳。
2. B 完成验证码和登录。
3. B 完成教务 SSO。
4. C 用 mock 数据完成课表本地管理。
5. D 用 mock 数据完成成绩、考表、校历页面。
6. B 提供真实 API。
7. C 接入课表导入。
8. D 接入成绩和考表。
9. E 执行回归测试和打包。

## 第一阶段不做的事

- 不做 OCR。
- 不做移动端小组件。
- 不做系统日历写入。
- 不做通知公告 WebView。
- 不做自动更新安装。
- 不做第二课堂。
- 不做余额查询。
- 不追求 Flutter 界面 1:1 复刻。

