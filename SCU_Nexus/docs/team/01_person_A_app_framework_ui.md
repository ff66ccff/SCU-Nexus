# 人员 A：应用框架与公共 UI 详细任务书

## 0. 本文档定位

人员 A 负责 `SCU_Nexus` 的 Qt 应用骨架、页面导航、公共 QML 组件、主题、登录页壳、设置页壳，以及所有功能模块可复用的 UI 状态组件。

第一阶段只承载四个业务入口：

- 课表管理
- 校历查询
- 考表查询
- 教务成绩

体测查询不进入本阶段导航，也不作为人员 D 的交付范围。后续如果恢复体测模块，应通过新增路由和独立 ViewModel 接入，不要挤占本阶段四个入口。

## 1. 迁移依据

主要参考 Flutter 项目中的这些文件和行为：

- `Bugaoshan/lib/app.dart`
- `Bugaoshan/lib/main.dart`
- `Bugaoshan/lib/pages/home_page.dart`
- `Bugaoshan/lib/pages/auth/scu_login_page.dart`
- `Bugaoshan/lib/pages/profile/profile_page.dart`
- `Bugaoshan/lib/pages/settings/software_setting_page.dart`
- `Bugaoshan/lib/widgets/common/loading_widgets.dart`
- `Bugaoshan/lib/widgets/common/login_required_widget.dart`
- `Bugaoshan/lib/widgets/common/retryable_error_widget.dart`
- `Bugaoshan/lib/widgets/dialog/dialog.dart`

参考原则：

- 借鉴状态流和交互意图，不要求 1:1 复刻 Flutter 视觉。
- Qt/QML 中不要直接写业务 HTTP、数据库 SQL、课表解析、成绩接口等逻辑。
- A 提供“壳”和公共交互，B/C/D 提供业务 ViewModel。

## 2. 推荐文件归属

人员 A 拥有以下路径或同等职责的文件：

```text
SCU_Nexus/
  CMakeLists.txt
  main.cpp
  src/
    app/
      AppController.h
      AppController.cpp
      Router.h
      Router.cpp
      AppSettings.h
      AppSettings.cpp
      ThemeManager.h
      ThemeManager.cpp
      ToastManager.h
      ToastManager.cpp
      DialogManager.h
      DialogManager.cpp
      ServiceRegistry.h
      ServiceRegistry.cpp
    qml/
      App.qml
      MainShell.qml
      LoginPage.qml
      SettingsPage.qml
      components/
        AppButton.qml
        AppDialog.qml
        AppTextField.qml
        DataTable.qml
        EmptyView.qml
        ErrorView.qml
        FormRow.qml
        IconButton.qml
        LoadingView.qml
        ModuleHeader.qml
        SearchBar.qml
        SectionHeader.qml
        ToastHost.qml
      styles/
        Colors.qml
        Metrics.qml
        Typography.qml
```

注意：

- 若当前项目还没有 `src/` 和 `qml/` 目录，先建立最小结构，不要一次性重构到过深。
- `main.cpp` 和 `CMakeLists.txt` 可以逐步从现有 Hello World 调整为模块化工程。
- 业务页面 QML 文件由 C/D 负责，但 A 负责让这些页面能注册到导航和统一布局中。

## 3. CMake 与工程骨架

### 3.1 Qt 模块

第一阶段 CMake 必须支持：

- `Qt6::Core`
- `Qt6::Gui`
- `Qt6::Qml`
- `Qt6::Quick`
- `Qt6::QuickControls2`
- `Qt6::Network`
- `Qt6::Sql`

测试阶段由人员 E 添加：

- `Qt6::Test`

可预留但第一阶段不强依赖：

- `Qt6::WebEngineQuick`
- `Qt6::Svg`

### 3.2 C++ 标准

建议使用 C++20。如果团队环境存在编译器差异，最低保证 C++17。

`CMakeLists.txt` 中应明确：

```cmake
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
```

### 3.3 QML 模块

QML URI 建议保持：

```text
SCU_Nexus
```

人员 A 需要把公共组件加入 `qt_add_qml_module()`。业务页面后续由 C/D 添加，A 不要把业务页面写死在 `Main.qml` 内。

## 4. 应用启动流程

`main.cpp` 只负责：

1. 创建 `QGuiApplication` 或 `QApplication`。
2. 设置组织名、应用名、版本号。
3. 设置 Quick Controls 样式。
4. 初始化 `AppController`。
5. 注册 C++ 类型到 QML。
6. 加载 `App.qml`。

`AppController` 负责：

1. 初始化 `AppSettings`。
2. 初始化 `ThemeManager`。
3. 调用 B 的认证服务恢复登录状态。
4. 调用 C 的课表 Repository 初始化本地数据库。
5. 创建或持有 ViewModel 实例。
6. 将登录状态、启动状态、全局错误暴露给 QML。

建议信号：

```cpp
signals:
    void startupFinished();
    void startupFailed(QString message);
    void loginStateChanged(bool loggedIn);
    void sessionExpired(QString message);
```

建议 QML 可读属性：

```cpp
Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)
Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY loggedInChanged)
Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
```

## 5. 路由与导航

### 5.1 路由枚举

`Router` 至少定义：

```cpp
enum class AppRoute {
    Login,
    Schedule,
    AcademicCalendar,
    ExamPlan,
    Grades,
    Settings
};
```

### 5.2 路由行为

必须支持：

- `navigate(route)`
- `replace(route)`
- `goBack()`
- `currentRoute`
- `canGoBack`
- `routeTitle`

登录态规则：

- `Schedule` 可以离线进入，显示本地课表；在线导入时再要求登录。
- `AcademicCalendar` 不要求登录。
- `ExamPlan` 要求登录。
- `Grades` 要求登录。
- `Settings` 不要求登录，但退出登录按钮只有登录后可用。
- 登录失效时，当前页面可以继续显示缓存，但刷新动作应显示登录失效提示。

### 5.3 MainShell

主窗口默认尺寸：

```text
width: 1100
height: 760
minimumWidth: 900
minimumHeight: 620
```

布局建议：

- 左侧导航栏：四个业务入口 + 设置。
- 顶部区域：当前页面标题、刷新按钮占位、登录状态入口。
- 中央区域：业务页面 Loader。
- 底部或右下角：ToastHost。

导航项顺序：

1. 课表
2. 校历
3. 考表
4. 成绩
5. 设置

### 5.4 页面生命周期

页面切换时：

- 不重新创建全局服务。
- 不销毁 Repository。
- 不重新登录。
- ViewModel 可以保留缓存；页面可重新绑定状态。

QML 中不要直接 `new` 网络服务。统一通过 `AppController` 暴露的 ViewModel 或 QML 单例访问。

## 6. 公共状态组件

所有业务页面必须复用 A 提供的状态组件。

### 6.1 LoadingView

属性：

```qml
property string text
property bool compact
```

行为：

- 全页加载时居中显示。
- 局部加载时可使用 compact。
- 不阻塞窗口关闭。

### 6.2 EmptyView

属性：

```qml
property string title
property string description
property string actionText
signal actionTriggered()
```

使用场景：

- 无课表。
- 无考试安排。
- 暂无教务成绩。
- 校历图片列表为空。

### 6.3 ErrorView

属性：

```qml
property string title
property string message
property bool canRetry
property string retryText
signal retry()
```

错误文案来源：

- B/C/D 的 ViewModel 暴露用户可读错误。
- A 不做错误类型判断，只负责显示。

### 6.4 LoginRequiredView

属性：

```qml
property string message
signal loginRequested()
```

用于：

- 考表未登录。
- 成绩未登录。
- 课表在线导入未登录。

校历页面不得显示 LoginRequiredView。

## 7. 表单与按钮组件

### 7.1 AppButton

支持：

- `primary`
- `secondary`
- `danger`
- `ghost`
- `busy`
- `enabled`

按钮中应支持图标。建议使用 Qt 内置图标资源或后续统一图标字体，不要每个页面自行画 SVG。

### 7.2 AppTextField

支持：

- `label`
- `placeholder`
- `errorText`
- `passwordMode`
- `clearable`

登录页和课程编辑页都应复用。

### 7.3 DataTable

第一阶段只需要轻量表格：

- 表头固定。
- 行高稳定。
- 支持空数据。
- 支持横向滚动。

主要供考表和成绩详情使用；课表网格由 C 单独实现。

## 8. 登录页壳

人员 A 负责 `LoginPage.qml` 的 UI，人员 B 负责 AuthViewModel 和实际登录逻辑。

登录页字段：

- 学号/账号
- 密码
- 验证码图片
- 验证码输入框
- 刷新验证码按钮
- 登录按钮
- 错误提示

交互要求：

1. 页面进入时调用 B 的 `fetchCaptcha()`。
2. 验证码图片加载失败时显示重试。
3. 登录按钮 busy 时禁用输入。
4. 登录成功后跳转到上次目标页，默认课表页。
5. 登录失败不清空账号和密码，只清空验证码文本并刷新验证码。

明确不做：

- 不接入 OCR。
- 不保存明文密码。
- 不在 QML 中拼接口 URL。
- 不在 QML 中处理 SM2 加密。

## 9. 设置页

第一阶段设置页包含：

- 登录状态卡片。
- 退出登录。
- 清除本地课表缓存。
- 清除查询缓存。
- 主题设置：跟随系统、浅色、深色。
- 查看版本号。

职责边界：

- “清除本地课表缓存”调用 C 的 `ScheduleViewModel.clearAllCourseData()`。
- “清除查询缓存”调用 D 的 `QueryCacheViewModel.clearAll()` 或等价接口。
- “退出登录”调用 B 的 `AuthViewModel.logout()`。
- A 只负责页面布局和按钮连接。

## 10. 主题与视觉规范

设计目标：

- 工具型校园客户端，信息密度适中。
- 不做营销型首页。
- 不使用大幅装饰渐变作为主视觉。
- 不让页面卡片套卡片。
- 各模块保持同一套间距、字号、状态组件。

建议尺寸：

```text
pagePadding: 20
sectionGap: 16
controlHeight: 36
cardRadius: 8
smallRadius: 4
navWidth: 180
```

文本层级：

- 页面标题：20-24 px。
- 区块标题：16-18 px。
- 表格/列表正文：13-15 px。
- 辅助说明：12-13 px。

颜色要求：

- 支持系统浅色/深色。
- 主题色统一由 `ThemeManager` 输出。
- 课程颜色由 C 的课表模块控制，A 不指定业务颜色池。

## 11. 与其他人员的接口

### 11.1 A 与 B

B 交付：

```cpp
class AuthViewModel : public QObject {
    Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY loggedInChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QUrl captchaImageUrl READ captchaImageUrl NOTIFY captchaChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)
    Q_INVOKABLE void fetchCaptcha();
    Q_INVOKABLE void login(QString username, QString password, QString captchaText);
    Q_INVOKABLE void logout();
};
```

A 使用该 ViewModel，不直接依赖 `ScuAuth` 内部类。

### 11.2 A 与 C

C 交付：

- `SchedulePage.qml` 或可被 Loader 加载的组件。
- `ScheduleViewModel`。
- `clearAllCourseData()`。
- `hasSchedule`、`loading`、`errorMessage` 等状态。

A 只负责注册路由和放入 MainShell。

### 11.3 A 与 D

D 交付：

- `AcademicCalendarPage.qml`
- `ExamPlanPage.qml`
- `GradesPage.qml`
- 各自 ViewModel。

A 只提供公共状态组件、页面标题区域、导航入口。

### 11.4 A 与 E

E 提供：

- UI 冒烟测试用例。
- 手工验收清单。
- 打包时需要包含的 QML 模块列表。

A 需要保证 QML 文件命名和模块注册稳定，避免打包阶段找不到资源。

## 12. 开发步骤

### 阶段 A1：从 Hello World 改为应用壳

任务：

1. 建立 `src/app` 和 `qml` 目录。
2. 改造 `CMakeLists.txt`，引入必要 Qt 模块。
3. 改造 `main.cpp`，加载 `App.qml`。
4. 创建 `AppController` 空实现。
5. 创建 `MainShell.qml`。
6. 左侧导航显示四个业务入口和设置。
7. 页面区先用占位组件。

验收：

- 应用启动不再显示 Hello World。
- 窗口尺寸符合要求。
- 点击导航能切换占位页面。

### 阶段 A2：公共组件

任务：

1. 实现 LoadingView。
2. 实现 EmptyView。
3. 实现 ErrorView。
4. 实现 LoginRequiredView。
5. 实现 ToastHost。
6. 实现 AppButton、AppTextField、SectionHeader。
7. 在占位页面中演示三态。

验收：

- C/D 可以直接引用组件。
- 深色/浅色下文本可读。
- 错误视图和空视图尺寸稳定。

### 阶段 A3：登录页壳

任务：

1. 创建 LoginPage。
2. 接入 B 的 AuthViewModel 属性。
3. 实现验证码图片显示。
4. 登录成功后返回原路由。
5. 登录失败显示错误。

验收：

- B 提供 mock ViewModel 时可完整演示登录流程。
- 输入 busy 状态不会重复提交。

### 阶段 A4：设置页

任务：

1. 显示版本。
2. 显示登录状态。
3. 接入退出登录。
4. 接入清缓存按钮。
5. 接入主题切换。

验收：

- 设置页不直接操作数据库和网络。
- 所有危险操作有确认弹窗。

### 阶段 A5：接入真实业务页

任务：

1. 替换课表占位为 C 的页面。
2. 替换校历、考表、成绩占位为 D 的页面。
3. 确认登录保护逻辑。
4. 确认切页不丢状态。

验收：

- 四个入口可达。
- 未登录访问考表/成绩时显示统一登录提示。
- 校历不要求登录。
- 课表本地页面可离线进入。

## 13. 验收标准

人员 A 完成时必须满足：

- 项目能在 Qt Creator 中打开并运行。
- 主窗口、导航、设置页、登录页存在。
- 四个业务入口清晰可见。
- 公共 Loading/Empty/Error/LoginRequired/Toast 组件可复用。
- QML 不含业务 HTTP URL。
- QML 不含 SQL。
- QML 不含 SM2 加密或 Cookie 处理。
- 页面切换不重新初始化全局服务。
- 深色/浅色主题均可用。

## 14. 文档间冲突检查结果

本任务书与其他人员边界如下：

- A 拥有应用壳和公共组件。
- B 拥有认证、网络、远端 API。
- C 拥有课表模型、课表数据库、课表页面。
- D 拥有校历、考表、成绩页面及其 ViewModel。
- E 拥有测试、打包、验收流程。

不存在职责重叠的点：

- A 不实现业务 API。
- A 不定义课表数据库。
- A 不解析教务成绩、考表或校历响应。
- A 不写打包脚本。

需要持续协调的点：

- A 改公共组件属性名时必须通知 C/D。
- A 改路由枚举时必须通知 C/D/E。
- A 改 QML 模块 URI 时必须通知所有人。
