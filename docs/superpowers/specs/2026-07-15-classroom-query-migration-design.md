# 课表与校历来源标注及教室查询迁移设计

## 目标

本次改造包含三个前端结果：

1. 课表页面展示“数据来自 http://zhjw.scu.edu.cn/student/courseSelect/thisSemesterCurriculum/index”，并允许用户直接在系统浏览器打开该地址。
2. 校历页面展示“数据来自 https://jwc.scu.edu.cn/cdxl.htm”，并允许用户直接在系统浏览器打开该地址。
3. 用从 `D:\SCU-Nexus\Bugaoshan-main` 迁移的完整“教室查询”功能替换现有用户可见的“考表”入口和页面。

## 约束

- 目标应用继续使用现有 Qt 6、C++ 和 QML 架构，不引入 Flutter 运行时或 WebView。
- 教室查询复用现有 `ScuAuthService -> ZhjwAuthService -> ZhjwApiService` 登录与会话链路。
- 不删除现有考表服务、模型和 ViewModel，避免影响既有测试或工作区中其他尚未提交的工作；只移除用户可见入口和页面装配。
- 保留 Bugaoshan 教室查询的完整核心交互：校区、教学楼、日期、节次筛选、教室列表和 12 节占用详情。
- 本次只修改与目标直接相关的文件，保留工作区已有无关改动。

## 方案选择

采用 Qt 原生完整迁移方案。Flutter 源代码作为数据契约和交互语义的参考，在目标工程中重写为 C++ 数据模型、API/解析逻辑、ViewModel 和 QML 页面。

未采用以下方案：

- 嵌入教务网页：违反现有 QML 不嵌入 Web 运行时的契约，且难以维持统一视觉和可测试状态。
- 仅提供教学楼与日期的简化查询：会丢失源功能中的节次筛选和教室详情，不符合“迁移教室查询功能”的完整语义。

## 页面与导航

### 数据来源链接

课表页、校历页和教室查询页在各自模块标题附近新增一行来源说明。三处统一复用 `SourceAttribution`，以“数据来自”开头，完整显示各自要求中的 URL，并通过 QML `MouseArea` 调用 `Qt.openUrlExternally(url)`。鼠标悬停使用手型光标，链接使用主题强调色，确保视觉上可识别为可点击内容。

教室查询页参考课表页布局，在标题栏之后、查询操作之前放置全宽来源说明，并在校区、教学楼和教室三个查询层级始终显示。教室查询来源 URL 固定为 `http://zhjw.scu.edu.cn/student/teachingResources/classroomCurriculum/index`。

### 教室查询入口

- `AppNavigation.qml` 将“考表”替换为“教室查询”，路由名改为 `Classroom`。
- `Router` 新增或替换对应枚举、标题和字符串映射。
- `MainShell.qml` 将该路由映射到 `ClassroomPage.qml`，刷新按钮调用 `classroomViewModel.refresh()`。
- 教室查询仍是登录保护页面；未登录时显示“教室查询需要登录教务系统”。

现有 `ExamPlanPage.qml` 可继续留在资源中，但不会再由主导航或主壳加载。

## 数据模型与远端接口

### 远端数据结构

按 Bugaoshan 的字段契约建立以下 C++ DTO/模型：

- `ClassroomCampusDto`：`campusName`、`campusNumber`。
- `ClassroomBuildingDto`：`campusNumber`、`teachingBuildingNumber`、`teachingBuildingName`。
- `ClassroomInfoDto`：教室名称、编号、教学楼编号、校区编号、座位数、状态、类型、备注和是否可借用。
- `ClassroomTimeSlotDto`：教室编号、起始节次、连续节数和占用模块编号。
- `ClassroomQueryResultDto`：教室列表、占用时段、查询日期和教学周。

占用模块映射保持源项目语义：

- `06`：上课。
- `07`：考试。
- `14`：实验。
- `room`：借用。
- 其他或缺失记录：空闲。

连续占用通过 `sessionstart` 和 `continuingsession` 展开到对应节次，最终为每间教室生成第 1–12 节状态。

### API

`ZhjwApiService` 增加两个异步接口：

1. GET `/student/teachingResources/classroomUseStatus/index`，从 HTML 中 `xqList` 与 `jxlList` 隐藏字段解析校区和教学楼列表。
2. POST `/student/teachingResources/classroomUseStatus/jasInfo`，提交 `xqh`、`jxlh`、`jslx`、`jasm`、`zwFrom`、`zwTo` 和 `searchDate`，解析教室使用情况 JSON。

请求继续使用现有 SSO 获取、会话失效识别和单次重试机制。解析失败返回明确的 `ApiError`，不能把格式异常误判为空结果。

`ZhjwQueryService` 增加教室查询所需的窄接口，使 ViewModel 测试可注入假服务；真实 `ZhjwApiQueryService` 只转发 API 结果，不承担界面状态和筛选逻辑。

## ViewModel

新增 `ClassroomViewModel`，职责边界如下：

- 管理 `campus`、`building`、`room` 三种页面层级。
- 初次加载校区和教学楼索引；选择校区后只展示对应教学楼。
- 选择教学楼后，按当前日期获取教室使用情况。
- 日期默认今天，允许选择今天前 7 天到今天后 30 天；日期变化后自动重新查询当前教学楼。
- 管理起止节次筛选，范围固定为 1–12，并保证起始节次不大于结束节次。
- 对当天查询提供“当前节次空闲”快捷筛选；非当天查询不展示该快捷筛选。
- 将教室状态转换为 QML 可直接消费的 `QVariantList`/`QVariantMap`，包括每节状态、状态文本、座位数、备注和可借用标记。
- 使用现有 `QueryState` 表达 `Idle`、`Loading`、`Loaded`、`Empty`、`Error` 和 `LoginRequired`。
- 忽略加载期间的重复请求，避免竞态和重复网络调用。

本功能不新增离线缓存。教室空闲状态与日期强相关，展示旧缓存容易误导；失败时保留已显示结果仅限同一次页面会话，并给出错误提示。

## QML 页面

新增 `qml/pages/classroom/ClassroomPage.qml`，并按职责拆分必要组件：

- 校区层：卡片列表，选择后进入教学楼层。
- 教学楼层：展示所选校区的教学楼列表，并支持返回校区层。
- 教室层：顶部显示教学楼、教学周、查询日期和结果数量；提供日期、今天、当前节次空闲和节次范围筛选。
- 教室卡片：显示名称、座位数和第 1–12 节的彩色状态标记。
- 教室详情：响应式对话框，显示校区、教学楼、查询日期、教学周、座位、备注、可借用状态，以及 12 节逐项状态。

颜色语义与 Bugaoshan 保持一致：空闲为绿色、上课为红色、考试为橙色、实验为紫色、借用为琥珀色；同时始终显示文字或工具提示，不能只靠颜色表达状态。

## 错误与空状态

- 未登录：由主壳登录保护视图拦截，并提供跳转登录按钮。
- 索引加载失败：显示可重试错误视图。
- 指定教学楼查询失败：停留在教室层并允许重试，不丢失已选校区、教学楼和日期。
- 返回空教室列表：显示“暂无教室数据”；启用节次筛选后无匹配项时显示“该节次范围暂无空闲教室”。
- 会话失效：使用 `LoginRequired` 状态和现有会话失效流程，引导重新登录。
- 解析异常：显示稳定的用户提示，详细响应只进入已脱敏日志。

## 测试与验收

实施采用测试驱动流程，先看到每个新增测试因功能缺失而失败，再完成最小实现。

### C++ 测试

- 解析有效的校区和教学楼 HTML。
- 拒绝缺少 `xqList` 或 `jxlList` 的异常页面。
- 解析教室信息、日期、教学周和占用时段。
- 正确展开连续节次并映射五种状态。
- 验证 POST URL、表单字段、请求头与会话重试。
- 验证 ViewModel 的层级切换、日期重查、节次筛选、空状态、错误状态、登录状态和重复请求保护。

### QML 契约测试

- 课表和校历页面包含完整来源 URL、`Qt.openUrlExternally` 和可点击区域。
- 导航、Router 和 MainShell 使用 `Classroom`，用户可见文本不再显示主入口“考表”。
- 教室页面包含校区、教学楼、日期、节次筛选、12 节状态、详情和完整查询状态。
- QML 继续不包含 `WebView`、`WebEngine` 或 `QtWebChannel`。

### 最终验证

- 构建目标项目。
- 运行新增的聚焦测试。
- 运行完整 CTest 测试集。
- 检查最终差异只包含本目标相关改动，且未覆盖工作区已有无关修改。

## 非目标

- 不删除考表后端、缓存或其既有单元测试。
- 不将 Flutter 依赖、资源或本地化框架复制到 Qt 工程。
- 不增加教室收藏、搜索名称、座位数筛选或教室类型筛选等源页面当前未暴露的扩展功能。
- 不改变课表导入和校历抓取的数据逻辑；两处只增加来源说明和外部跳转。
