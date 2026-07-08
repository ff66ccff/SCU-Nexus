# 人员 E：测试、打包与冲突检查详细任务书

## 0. 本文档定位

人员 E 负责第一阶段质量保障和交付：

- 自动化测试框架
- 单元测试
- ViewModel 测试
- 手工验收清单
- Windows 打包
- 干净环境验证
- 文档和代码职责冲突检查
- 第二阶段可选功能预研记录

人员 E 不负责实现业务功能，但需要推动 A/B/C/D 提供可测试接口和测试夹具。

第一阶段四个功能以最新口径为准：

- 课表管理
- 校历查询
- 考表查询
- 教务成绩

体测查询纳入后续可选项，不作为第一阶段验收内容。

## 1. 迁移依据

可复用 Flutter 测试思路：

- `Bugaoshan/test/widget_test.dart`
- `Bugaoshan/test/exam_plan_test.dart`
- `Bugaoshan/test/calendar_event_utils_test.dart`
- `Bugaoshan/test/ics_service_test.dart`
- `Bugaoshan/lib/models/scheme_score.dart`
- `Bugaoshan/lib/providers/grades_provider.dart`

可参考 Flutter 工程能力：

- `Bugaoshan/.github/workflows/build-windows.yml`
- `Bugaoshan/windows/`
- `Bugaoshan/pubspec.yaml`

Qt 项目以 `SCU_Nexus` 为准，不要改 Flutter 项目的构建配置。

## 2. 推荐文件归属

人员 E 拥有：

```text
SCU_Nexus/
  tests/
    CMakeLists.txt
    fixtures/
      jwxt_schedule_sample.json
      exam_plan_sample.html
      academic_calendar_list_sample.html
      academic_calendar_detail_sample.html
      grades_scheme_scores_sample.json
      grades_passing_scores_sample.json
      grades_scheme_scores_empty.json
      grades_passing_scores_empty.json
    unit/
      test_course.cpp
      test_schedule_config.cpp
      test_schedule_repository.cpp
      test_jwxt_schedule_parser.cpp
      test_cookie_http_client.cpp
      test_zhjw_parsers.cpp
      test_academic_calendar_parser.cpp
      test_exam_plan_viewmodel.cpp
      test_grade_models.cpp
      test_grades_viewmodel.cpp
    integration/
      test_app_startup.cpp
      test_qml_smoke.cpp
  scripts/
    package_windows.ps1
    verify_package.ps1
    conflict_check.py
  docs/
    qa/
      manual_acceptance_checklist.md
      release_checklist.md
      optional_modules_research.md
```

如果团队暂时不想新增脚本，也至少要交付测试文件和手工验收文档。

## 3. 测试框架

### 3.1 CMake

在 `SCU_Nexus/tests/CMakeLists.txt` 中启用：

```cmake
find_package(Qt6 REQUIRED COMPONENTS Test Core Sql Network Qml Quick)
enable_testing()
```

每个测试可单独 executable：

```cmake
qt_add_executable(test_course unit/test_course.cpp)
target_link_libraries(test_course PRIVATE Qt6::Test scu_nexus_core)
add_test(NAME test_course COMMAND test_course)
```

建议把业务 C++ 编译成可被测试链接的静态库：

```text
scu_nexus_core
```

如果 A 当前还没有拆库，E 需要提出改造需求，但不要直接大改 A 的构建文件，先协商。

### 3.2 测试数据

fixture 必须脱敏：

- 不包含真实学号。
- 不包含真实姓名。
- 不包含真实 token。
- 不包含 Cookie。
- 课程名、考试名可以使用公开或虚构样例。

命名规则：

```text
feature_case_expected.ext
```

示例：

```text
exam_plan_two_cards.html
grades_scheme_scores_mixed_terms.json
grades_passing_scores_failed_course.json
jwxt_schedule_odd_even_weeks.json
```

## 4. A 模块测试要求

### 4.1 启动测试

检查：

- 应用可创建 `AppController`。
- QML 模块能加载。
- MainShell 存在。
- 初始路由合法。

### 4.2 公共组件冒烟测试

至少覆盖：

- LoadingView 可实例化。
- EmptyView 可实例化。
- ErrorView 可触发 retry signal。
- LoginRequiredView 可触发 loginRequested signal。
- ToastHost 可显示和自动消失。

### 4.3 导航测试

用 fake ViewModel 验证：

- 可以切换到课表。
- 可以切换到校历。
- 可以切换到考表。
- 可以切换到成绩。
- 可以切换到设置。
- 未登录考表/成绩显示登录提示。
- 校历未登录仍显示页面。

## 5. B 模块测试要求

### 5.1 CookieHttpClient

测试：

- 精确 host Cookie 发送。
- 父域 Cookie 发送。
- 无关域 Cookie 不发送。
- 多 Set-Cookie 解析。
- Set-Cookie 中 Expires 包含逗号时不误拆。
- 302 跳转每跳保存 Cookie。
- 超过重定向上限报错。

### 5.2 Auth

测试：

- token 未过期判断。
- token 过期判断。
- logout 清理状态。
- captcha data URL 解码。
- captcha 缺字段报错。

不要在 CI 中打真实登录接口。

### 5.3 Zhjw parser

测试：

- 当前周正则 `第(\d+)周`。
- 学期 option 解析。
- 考表 HTML 解析。
- schemeScores callback URL 解析。
- allPassingScores callback URL 解析。
- 302/空 body/login HTML 判定为 session expired。

## 6. C 模块测试要求

### 6.1 Course

覆盖：

- `isInWeekRange(week)`。
- `isActiveInWeek(week)`。
- Every/Odd/Even。
- 同日同节次同周冲突。
- 不同星期不冲突。
- 节次不重叠不冲突。
- 周次不重叠不冲突。
- 单双周互斥不冲突。
- `excludeId` 生效。

### 6.2 ScheduleConfig

覆盖：

- `sectionsPerDay = morning + afternoon + evening`。
- 开学前 currentWeek = 1。
- 开学当天 currentWeek = 1。
- 第 8 天 currentWeek = 2。
- 超过 totalWeeks clamp 到 totalWeeks。
- 江安时间表关键节点。
- 望江/华西时间表关键节点。

### 6.3 Repository

使用临时数据库路径。

覆盖：

- 空库初始化不创建默认课表。
- addSchedule 后 allSchedules 增加。
- switchSchedule 写 metadata。
- delete current schedule 后切换。
- delete last schedule 后 currentScheduleId 为空。
- addCourse/updateCourse/deleteCourse。
- replaceScheduleCourses。
- clearAllCourseData。

### 6.4 JwxtScheduleParser

覆盖：

- 连续周 bit string。
- 单周 bit string。
- 全奇数周 -> Odd。
- 全偶数周 -> Even。
- 混合周 -> Every。
- 空 timeAndPlaceList。
- classWeek 全 0 跳过。
- 非法节次报错。

## 7. D 模块测试要求

### 7.1 AcademicCalendar

覆盖：

- 列表页解析多个学年。
- 详情页解析多张图片。
- 无图片返回空。
- 非 UTF-8 fallback。
- 缓存读取。
- 刷新失败保留缓存。

### 7.2 ExamPlan

覆盖：

- DTO 转页面模型。
- `isPast` 正常解析。
- `isPast` 日期解析失败。
- 排序。
- 空列表 state = Empty。
- 未登录 state = LoginRequired。

### 7.3 Grades

覆盖：

- `GradeCourseItem` 字段映射。
- `passed` 判断。
- `hasEffectiveScore` 判断。
- `SchemeScoreSummary.fromJson`。
- `PassingScoreResult.fromJson`。
- 方案成绩按学年学期分组。
- 及格成绩按学年学期排序。
- GPA 计算。
- 必修 GPA 计算。
- 已修学分计算。
- 总通过门数计算。
- 学分加权均分计算。
- 必修学分加权均分计算。
- 必修/选修/任选学分统计。
- 搜索课程名过滤。
- 课程属性过滤。
- 自定义统计选择课程后重新计算。
- 缓存读取。
- 刷新失败保留缓存。
- 未登录 state = LoginRequired。

## 8. 手工验收清单

在 `docs/qa/manual_acceptance_checklist.md` 中维护。第一阶段必须覆盖：

### 8.1 启动和基础

- 首次启动应用。
- 主窗口尺寸正常。
- 四个导航入口可见：课表、校历、考表、成绩。
- 设置页可见。
- 浅色/深色主题切换。
- 重启后窗口基本状态正常。

### 8.2 登录

- 获取验证码成功。
- 刷新验证码成功。
- 验证码错误时提示清晰。
- 密码错误时提示清晰。
- 断网登录时提示清晰。
- 登录成功后回到目标页面。
- 退出登录后考表/成绩需要重新登录。

### 8.3 课表

- 无课表时显示空状态。
- 新建空课表。
- 新增课程。
- 编辑课程。
- 删除课程。
- 创建冲突课程被阻止。
- 删除课表。
- 重启后课程仍存在。
- 在线导入单个学期。
- 同名导入选择添加后缀。
- 同名导入选择更新已有。
- 同步当前教学周。

### 8.4 校历

- 未登录可打开。
- 可加载学年列表。
- 可切换学年。
- 可显示图片。
- 图片点击可放大。
- 断网时有错误态。
- 有缓存时断网仍显示旧数据。

### 8.5 考表

- 未登录显示登录提示。
- 登录后刷新考表。
- 无考试显示空状态。
- 考试卡片字段完整。
- 已结束考试弱化显示。
- 会话过期后刷新提示重新登录。
- 有缓存时刷新失败不清空旧数据。

### 8.6 教务成绩

- 未登录显示登录提示。
- 登录后进入成绩页面。
- 方案成绩 Tab 可刷新。
- 及格成绩 Tab 可刷新。
- 自定义统计 Tab 可使用方案成绩数据。
- 课程搜索可用。
- 课程属性筛选可用。
- 方案成绩 summary 显示 GPA、必修 GPA、通过/未通过、均分、学分。
- 及格成绩 summary 显示总 GPA、累计学分、通过门数、学期数。
- 自定义统计选择课程后指标变化。
- 会话过期后刷新提示重新登录。
- 有缓存时刷新失败不清空旧数据。

### 8.7 打包

- Release 包在开发机可运行。
- Release 包在干净 Windows 环境可运行。
- QML 模块不缺失。
- SQLite 驱动可用。
- HTTPS 请求可用。

## 9. Windows 打包

### 9.1 前置条件

确认：

- 使用 Release 构建。
- Qt 版本与开发环境一致。
- MinGW/MSVC 运行库选择明确。
- 应用 exe 可直接启动。

### 9.2 windeployqt

流程：

1. 创建空发布目录。
2. 复制 `appSCU_Nexus.exe`。
3. 运行 `windeployqt`。
4. 指定 QML import 路径。
5. 确认 `sqldrivers` 包含 SQLite。
6. 确认 TLS/OpenSSL 依赖存在。
7. 复制许可证文本。
8. 复制必要资源。

示例命令由 E 根据本机 Qt 路径写入 `scripts/package_windows.ps1`。

### 9.3 包内必须包含

- exe。
- Qt 运行库 DLL。
- QML 模块。
- platforms 插件。
- imageformats 插件。
- sqldrivers 插件。
- tls 插件或 OpenSSL DLL。
- 资源文件。
- LICENSE。
- README 或简短使用说明。

### 9.4 干净环境验证

环境：

- 未安装 Qt。
- 未安装开发工具。
- 普通用户权限。

验证：

- 双击启动。
- 关闭重启。
- 创建课表。
- 打开校历。
- 登录页验证码能显示。
- SQLite 写入成功。
- HTTPS 不报 TLS 错误。

## 10. 冲突检查职责

### 10.1 文档职责冲突

E 需要持续检查五份任务书的边界：

```text
A: 应用壳、公共 UI、路由、设置页壳
B: 认证、网络、教务 API
C: 课表模型、课表数据库、课表页面、课表导入解析
D: 校历、考表、教务成绩页面和查询缓存
E: 测试、打包、验收、冲突检查
```

当前五份文档已调整的冲突点：

- 原分工中本阶段需要做的是教务成绩，不是体测成绩。
- 体测查询移入后续可选项。
- 教务成绩进入第一阶段，由 B+D 实现。
- 校历公开页不走统一认证；D 实现校历抓取，B 只提供普通网络能力。
- 考表 HTML 解析归 B，D 不重复解析。
- 成绩 callback URL 提取和请求归 B，成绩模型与统计归 D。
- 课表教务 JSON 到 Course 的解析归 C，B 不写数据库。
- 设置页清缓存按钮由 A 展示，分别调用 C/D 的清理接口。

### 10.2 代码职责冲突

建议脚本 `scripts/conflict_check.py` 检查：

- QML 中是否出现 `http://` 或 `https://`。
- QML 中是否出现 SQL 关键字。
- D 页面中是否直接 include `CookieHttpClient`。
- C 模块中是否直接 include `ScuAuthService`。
- B 模块中是否 include `ScheduleRepository`。
- B 模块中是否 include `GradeStatisticsService`。
- 是否重复定义 `Course`。
- 是否重复定义 `GradeCourseItem`。
- 是否重复定义 `ExamPlanItemDto`。
- 是否有多个 `currentScheduleId` 常量。

可用简单文本规则先做，不需要复杂 AST。

### 10.3 合并前检查

每次合并前 E 需要确认：

- 新文件路径符合人员归属。
- 新 public 接口有测试或手工验收项。
- DTO 字段变更已通知依赖方。
- QML 入口名变更已通知 A。
- 数据库 schema 变更有迁移说明。
- 打包脚本仍能找到 QML 文件。

## 11. 可选功能预研

原分工中的第二阶段预研仍由 E 维护，但优先级按本次第一阶段完成后再排。

建议预研顺序：

1. 体测查询：SSO relay 类模块，功能相对单一，优先级高。
2. 培养方案：复用 B 的教务认证，优先级高。
3. 校园网设备查询。
4. 校园卡/网费查询。
5. 寝室电费/空调余额。
6. 第二课堂。

每个预研记录：

- 是否需要新增认证。
- 需要参考哪些 Flutter 文件。
- API 数量。
- 页面数量。
- 数据模型数量。
- 预计工作量。
- 最大风险。
- 是否影响现有 A/B/C/D 边界。

注意：成绩查询已不是可选预研项，它是第一阶段正式范围。

## 12. 开发步骤

### 阶段 E1：测试基础设施

1. 创建 tests 目录。
2. 接入 Qt Test。
3. 建立 fixtures。
4. 写一个最小测试并接入 CTest。
5. 与 A 确认是否需要拆 core library。

### 阶段 E2：模型和解析测试

1. 写 Course 测试。
2. 写 ScheduleConfig 测试。
3. 写 Cookie 测试。
4. 写 Jwxt parser 测试。
5. 写 Exam parser/ViewModel 测试。
6. 写 Calendar parser 测试。
7. 写 Grade model 和 GradesViewModel 测试。

### 阶段 E3：手工验收文档

1. 创建手工验收清单。
2. 与 A/B/C/D 确认每项可操作。
3. 标记阻塞项。
4. 每轮联调后更新结果。

### 阶段 E4：打包脚本

1. 写 Release 构建步骤。
2. 写 windeployqt 脚本。
3. 写包验证脚本。
4. 在干净环境测试。

### 阶段 E5：冲突检查

1. 建立职责检查脚本。
2. 每次合并前运行。
3. 把发现的问题反馈给对应人员。
4. 维护本文档的冲突规则。

## 13. 验收标准

人员 E 完成时必须满足：

- 测试工程可运行。
- 核心模型测试存在。
- 解析测试存在。
- Repository 测试存在。
- 成绩模型和统计测试存在。
- 手工验收清单存在。
- Windows 打包脚本存在。
- Release 包可在干净环境启动。
- 冲突检查规则存在。
- 已明确教务成绩为第一阶段正式范围。
- 已明确体测不在第一阶段范围。

## 14. 文档间冲突检查结果

本次五份文档检查结论：

- 无不可并行的职责冲突。
- 已修正“成绩/体测阶段范围”的冲突：以用户最新更正为准，教务成绩进入第一阶段，体测进入后续可选项。
- API 与 UI 边界清晰：B 提供远端服务，A/C/D 使用 ViewModel 接入。
- 数据边界清晰：C 管课表 SQLite，D 管查询缓存和成绩模型，B 不写业务数据。
- 打包和测试边界清晰：E 负责验证，不直接改业务职责。

需要团队在实现中继续关注：

- DTO 字段名变更。
- QML 入口名变更。
- 数据库 schema 变更。
- 公共组件属性变更。
- 认证错误类型变更。
