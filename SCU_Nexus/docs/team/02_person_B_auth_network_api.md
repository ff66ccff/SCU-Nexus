# 人员 B：认证、网络与教务 API 详细任务书

## 0. 本文档定位

人员 B 负责统一认证、Cookie HTTP 客户端、教务 SSO、教务 API，以及给 UI/ViewModel 使用的稳定异步接口。

第一阶段必须支撑四个业务模块：

- 课表管理：提供学期列表、课表原始 JSON、当前教学周。
- 校历查询：不需要认证；B 只提供可复用普通 HTTP 能力，具体校历抓取由 D 负责。
- 考表查询：提供教务考表 API。
- 教务成绩：提供方案成绩和及格成绩 API。

体测查询不进入第一阶段。体测 SSO、体测 API、体测通知、体测成绩都放入后续可选项，不作为本阶段验收内容。

## 1. 迁移依据

主要参考 Flutter 项目：

- `Bugaoshan/lib/services/auth/cookie_client.dart`
- `Bugaoshan/lib/services/auth/scu_auth.dart`
- `Bugaoshan/lib/services/auth/zhjw_auth.dart`
- `Bugaoshan/lib/services/auth/scu_exceptions.dart`
- `Bugaoshan/lib/services/api/zhjw_api_service.dart`
- `Bugaoshan/lib/providers/grades_provider.dart`
- `Bugaoshan/lib/models/scheme_score.dart`
- `Bugaoshan/lib/utils/sm2_crypto.dart`
- `Bugaoshan/lib/utils/constants.dart`

关键差异：

- Qt 版不迁移 OCR。
- Qt 版验证码由用户手动输入。
- Qt 版第一阶段不实现 FitnessAuth、FitnessApiService。
- 成绩接口仍走教务系统 `zhjw.scu.edu.cn`，复用 ZhjwAuth。

## 2. 推荐文件归属

人员 B 拥有：

```text
SCU_Nexus/src/
  core/
    network/
      CookieHttpClient.h
      CookieHttpClient.cpp
      HttpRequest.h
      HttpResponse.h
      NetworkError.h
      NetworkSettings.h
    crypto/
      Sm2Crypto.h
      Sm2Crypto.cpp
    logging/
      AuthLogger.h
      AuthLogger.cpp
  services/
    auth/
      AuthState.h
      AuthViewModel.h
      AuthViewModel.cpp
      ScuAuthService.h
      ScuAuthService.cpp
      ZhjwAuthService.h
      ZhjwAuthService.cpp
      AuthErrors.h
    api/
      ZhjwApiService.h
      ZhjwApiService.cpp
      ApiDtos.h
      ApiErrors.h
```

人员 B 不拥有：

- 课表 SQLite Repository。
- 课表导入后如何保存。
- 校历页面 UI。
- 考表页面 UI。
- 成绩页面 UI。
- 成绩统计模型。
- QML 公共组件。
- Windows 打包脚本。

## 3. 常量与基础地址

必须复用 Flutter 版核心地址：

```text
SCU 统一认证:
https://id.scu.edu.cn

教务系统:
http://zhjw.scu.edu.cn

教务 SSO:
https://id.scu.edu.cn/enduser/sp/sso/scdxplugin_jwt23?enterpriseId=scdx&target_url=index

校历公开页:
https://jwc.scu.edu.cn/cdxl.htm
```

`kDefaultUserAgent` 必须统一放在网络层，避免各模块复制。

建议：

```cpp
namespace ApiEndpoints {
constexpr auto kScuIdBase = "https://id.scu.edu.cn";
constexpr auto kZhjwBase = "http://zhjw.scu.edu.cn";
constexpr auto kJwcCalendar = "https://jwc.scu.edu.cn/cdxl.htm";
}
```

## 4. 异常和错误类型

参考 Flutter `ScuException` 体系，Qt 版建议建立统一错误：

```cpp
enum class ApiErrorType {
    Network,
    Timeout,
    ParseFailed,
    Unauthenticated,
    SessionExpired,
    ServiceUnavailable,
    RateLimited,
    InvalidCaptcha,
    InvalidCredential,
    Unknown
};
```

建议 DTO：

```cpp
struct ApiError {
    ApiErrorType type;
    QString message;
    int statusCode = 0;
    QString debugBody;
};
```

对 QML 暴露时不要暴露异常对象，只暴露：

- `errorType`
- `errorMessage`
- `debugMessage` 仅日志可见

## 5. CookieHttpClient

### 5.1 基本职责

对应 Flutter `CookieClient`。

必须实现：

1. 按 host 存储 Cookie。
2. 请求时只发送当前 host 或父域匹配的 Cookie。
3. 手动处理 302/301/303/307/308 重定向。
4. 每一跳保存 `Set-Cookie`。
5. 支持 GET/POST。
6. 支持 `application/x-www-form-urlencoded`。
7. 支持 JSON body。
8. 支持超时。
9. 支持一次网络异常重试。
10. 能被 ZhjwAuth 缓存复用。

### 5.2 Cookie 域匹配

最低实现：

```text
host == jarHost
host.endsWith("." + jarHost)
```

注意：

- 第一阶段可以忽略 Path 精确匹配，但应在注释中标明。
- `Set-Cookie` 可能以逗号分隔多个 Cookie，不能简单按所有逗号拆。
- 不要把所有域的 Cookie 混成一个全局 Header。

### 5.3 建议接口

```cpp
struct HttpResponse {
    int statusCode;
    QByteArray body;
    QMap<QString, QString> headers;
    QUrl finalUrl;
};

class CookieHttpClient : public QObject {
    Q_OBJECT
public:
    explicit CookieHttpClient(QObject* parent = nullptr);

    QFuture<HttpResponse> get(
        const QUrl& url,
        const QMap<QString, QString>& headers = {});

    QFuture<HttpResponse> post(
        const QUrl& url,
        const QByteArray& body,
        const QMap<QString, QString>& headers = {});

    QFuture<HttpResponse> followRedirects(
        const QUrl& url,
        const QMap<QString, QString>& headers = {},
        int maxRedirects = 10);

    void clearCookies();
    QString cookieHeaderForDebug() const;
};
```

如果团队不使用 `QFuture`，可以改为 signal/callback，但对外接口要保持异步，不得阻塞 UI 线程。

### 5.4 重定向细节

每一跳：

1. 构造请求。
2. 加当前目标域 Cookie。
3. 关闭 Qt 自动跳转，手动处理 Location。
4. 保存响应 Set-Cookie。
5. 如果是 3xx 且有 Location，resolve 相对路径。
6. 超过 `maxRedirects` 抛出 `ServiceUnavailable` 或等价错误。

## 6. ScuAuthService

### 6.1 状态机

建议状态：

```cpp
enum class AuthState {
    Unknown,
    Loading,
    Ready,
    Expired,
    Error
};
```

QML 只需要关心：

- `loggedIn`
- `loading`
- `errorMessage`

### 6.2 验证码

接口：

```cpp
struct CaptchaResult {
    QString code;
    QByteArray imageBytes;
    QString mimeType;
};
```

流程：

1. GET `https://id.scu.edu.cn/api/public/bff/v1.2/one_time_login/captcha?_enterprise_id=scdx&timestamp=...`
2. 解析 `data.captcha` 或 `data.image` 等字段。
3. 解析 `data.code`。
4. 如果图片是 data URL，去掉逗号前缀后 base64 decode。
5. 通过 AuthViewModel 暴露给 QML。

不做：

- 不调用 OCR。
- 不加载 `assets/universal-login-ocr.tflite`。

### 6.3 SM2 公钥和登录

流程复刻 Flutter `ScuAuth.login()`：

1. POST `.../sm2_key`，body `{}`。
2. 失败时最多重试 3 次。
3. 取 `data.publicKey` 和 `data.code`。
4. 使用 SM2 C1C2C3 方式加密密码。
5. POST `.../rest_token`。
6. body 包含：

```json
{
  "client_id": "1371cbeda563697537f28d99b4744a973uDKtgYqL5B",
  "grant_type": "password",
  "scope": "read",
  "username": "...",
  "password": "SM2密文",
  "_enterprise_id": "scdx",
  "sm2_code": "...",
  "cap_code": "...",
  "cap_text": "用户输入验证码"
}
```

7. `success != true` 时把服务端 message 显示给登录页。
8. 成功后保存 `access_token` 和登录时间戳。

### 6.4 Token 保存

优先：

- Windows Credential Manager 或 Qt Keychain。

如果第一阶段来不及：

- 临时使用 `QSettings`。
- 文档和代码注释中明确这是技术债。
- 不保存密码。

时间规则：

- 登录时间戳用秒。
- TTL 为 3600 秒。
- 超时后先尝试重新 bind session。
- 失败后通知 UI 登录失效。

### 6.5 bindSession

对应 Flutter `_doBindSession()`：

1. 创建 `CookieHttpClient`。
2. POST `https://id.scu.edu.cn/api/bff/v1.2/commons/session/save`。
3. Header 加 `Authorization: Bearer access_token`。
4. body `{}`。
5. 成功后缓存 client。

并发要求：

- 多个 API 同时请求时，只执行一次 bind。
- 可用 `QMutex`、`QPromise`、单飞任务等方式实现。

## 7. ZhjwAuthService

对应 Flutter `ZhjwAuth`。

流程：

1. 调用 `ScuAuthService.getClient()`。
2. 如果 SCU client 变了，清空教务缓存。
3. 使用同一个 CookieHttpClient follow redirects：

```text
https://id.scu.edu.cn/enduser/sp/sso/scdxplugin_jwt23?enterpriseId=scdx&target_url=index
```

4. Header 加 `Authorization: Bearer token`。
5. 成功后缓存该 client。

失效处理：

- API 检测到 302、空 body、登录页 HTML 时，调用 `invalidate()`。
- 自动重试一次。
- 再失败才向 ViewModel 抛 `Unauthenticated`。

## 8. ZhjwApiService

### 8.1 公共请求包装

所有教务 API 必须通过统一请求包装：

```cpp
template <typename Fn>
auto requestWithRetry(Fn fn);
```

逻辑：

1. 获取 ZhjwAuth client。
2. 执行 API。
3. 检测 session 过期。
4. 过期则 `ZhjwAuth.invalidate()`。
5. 重试一次。
6. 仍失败则抛 `Unauthenticated`。

### 8.2 Session 过期判断

复刻 Flutter：

- status code 是 302。
- body trim 后为空。
- body 以 `<` 开头且包含 `login`。

可补充：

- body 包含 `统一身份认证`。
- body 包含 `用户登录`。

### 8.3 fetchCurrentWeek

请求：

```text
GET http://zhjw.scu.edu.cn/
```

Header：

```text
Accept: text/html,*/*
Referer: http://zhjw.scu.edu.cn/
User-Agent: kDefaultUserAgent
```

解析：

```text
第(\d+)周
```

返回：

```cpp
int
```

失败：

- 没有匹配时抛 `ServiceUnavailable` 或 `ParseFailed`，消息为“无法获取当前周数，请检查教务系统状态”。

### 8.4 fetchSemesters

请求：

```text
GET http://zhjw.scu.edu.cn/student/courseSelect/calendarSemesterCurriculum/index
```

解析：

```html
<option value="...">...</option>
```

返回：

```cpp
struct SemesterDto {
    QString value;
    QString label;
};
```

注意：

- `label` 保留 “（当前）”，由 C 导入时清理。
- 返回顺序沿用服务端。

### 8.5 fetchJwxtSchedule

请求：

```text
POST http://zhjw.scu.edu.cn/student/courseSelect/thisSemesterCurriculum/ajaxStudentSchedule/callback
Content-Type: application/x-www-form-urlencoded; charset=UTF-8
Body: planCode=<semester.value>
```

返回：

```cpp
QJsonObject
```

职责边界：

- B 只负责拿到 JSON 并校验可解析。
- C 负责把 `xkxx` 转换成 `Course` 和 `ScheduleConfig`。

### 8.6 fetchExamPlan

请求：

```text
GET http://zhjw.scu.edu.cn/student/examinationManagement/examPlan/index
```

解析依据 Flutter `_parseExamCards()`：

- 用卡片块正则定位 `widget-box widget-color-*`。
- 课程名取 `<h5 class="widget-title smaller">`。
- 移除 `（已结束）` 标记。
- 周次取 `(\d+)周`。
- 日期取 `YYYY-MM-DD`。
- 星期取 `星期[一二三四五六日]`。
- 时间取 `HH:mm-HH:mm`。
- 地点取 `地点:&nbsp;...`，把 `&nbsp;` 替换成空格。
- 座位号取 `座位号:&nbsp;...`。
- 准考证号可为空。
- 提示信息取 `考试提示信息`，没有则为 `无`。

返回：

```cpp
struct ExamPlanItemDto {
    QString courseName;
    QString week;
    QString date;
    QString weekday;
    QString timeRange;
    QString location;
    QString seatNumber;
    QString ticketNumber;
    QString tip;
};
```

职责边界：

- B 负责 HTML 解析。
- D 负责排序、缓存、展示和 `isPast` UI 状态。

## 9. 教务成绩 API

成绩 API 与课表/考表同源，全部走 `ZhjwAuthService`。

### 9.1 fetchSchemeScores

用途：

- 方案成绩。
- 支持 D 生成“方案成绩”和“自定义统计”两个 Tab。

流程复刻 Flutter `ZhjwApiService.fetchSchemeScores()`：

1. GET 入口页：

```text
http://zhjw.scu.edu.cn/student/integratedQuery/scoreQuery/schemeScores/index
```

2. 检查 session 过期。
3. 从 HTML 中提取 callback path：

```regex
var\s+url\s*=\s*"(/student/integratedQuery/scoreQuery/[^/]+/schemeScores/callback)"
```

4. GET：

```text
http://zhjw.scu.edu.cn<callbackPath>
```

5. 解析 JSON。
6. 返回 `QJsonObject`。

Header：

```text
Accept: application/json, text/plain, */*
Referer: http://zhjw.scu.edu.cn/student/integratedQuery/scoreQuery/schemeScores/index
User-Agent: kDefaultUserAgent
```

错误：

- 入口页无法提取 callback path 时，如果 body 像登录页，抛 `Unauthenticated`。
- 否则抛 `ParseFailed`，消息为“无法从页面提取 schemeScores callback URL”。

### 9.2 fetchPassingScores

用途：

- 及格成绩。

流程复刻 Flutter `ZhjwApiService.fetchPassingScores()`：

1. GET 入口页：

```text
http://zhjw.scu.edu.cn/student/integratedQuery/scoreQuery/allPassingScores/index
```

2. 检查 session 过期。
3. 从 HTML 中提取 callback path：

```regex
var\s+url\s*=\s*"(/student/integratedQuery/scoreQuery/[^/]+/allPassingScores/callback)"
```

4. GET：

```text
http://zhjw.scu.edu.cn<callbackPath>
```

5. 解析 JSON。
6. 返回 `QJsonObject`。

错误：

- 入口页无法提取 callback path 时，如果 body 像登录页，抛 `Unauthenticated`。
- 否则抛 `ParseFailed`，消息为“无法从页面提取 allPassingScores callback URL”。

### 9.3 成绩 JSON 解析边界

B 只负责：

- 找 callback URL。
- 请求 callback。
- 确认响应是 JSON。
- 返回原始 `QJsonObject`。

D 负责：

- `SchemeScoreItem`。
- `SchemeScoreSummary`。
- `PassingScoreGroup`。
- `PassingScoreResult`。
- GPA、学分、均分、必修/选修统计。
- 缓存和页面状态。

这样做可以避免 B 和 D 同时维护一套成绩业务模型。

## 10. AuthViewModel

A 的登录页只接触 AuthViewModel。

建议属性：

```cpp
Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY loggedInChanged)
Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
Q_PROPERTY(bool captchaLoading READ captchaLoading NOTIFY captchaLoadingChanged)
Q_PROPERTY(QUrl captchaImageUrl READ captchaImageUrl NOTIFY captchaChanged)
Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)
```

建议方法：

```cpp
Q_INVOKABLE void fetchCaptcha();
Q_INVOKABLE void login(QString username, QString password, QString captchaText);
Q_INVOKABLE void logout();
Q_INVOKABLE void clearError();
```

验证码图片暴露方式：

- 可以写入临时内存 image provider。
- 或写入应用缓存目录后用 `file://` QUrl。
- 不要让 QML 自己 base64 decode。

## 11. 日志要求

必须记录：

- 登录状态变化。
- captcha 请求成功/失败，不记录图片内容。
- sm2_key 成功/失败，不记录密码。
- token 成功获取，不记录 token 内容，只记录长度。
- SSO redirect hop 数量和 host。
- 教务 session 失效。
- API 解析失败时记录 endpoint 和响应前 500 字符。

不得记录：

- 明文密码。
- 完整 access token。
- Cookie 完整值。
- 成绩完整原始 JSON。
- 身份证/姓名等敏感字段。

## 12. 单元测试要求

人员 B 至少提供：

- Cookie 精确域匹配。
- Cookie 父域匹配。
- 不发送无关域 Cookie。
- 多 Set-Cookie 解析。
- 302 重定向链保存 Cookie。
- `fetchCurrentWeek` 正则解析。
- 学期 option 解析。
- 考表 HTML 卡片解析。
- schemeScores callback URL 解析。
- allPassingScores callback URL 解析。
- session expired HTML 判断。
- Auth TTL 判断。

测试 HTTP 不打真实校园网，使用假响应。

## 13. 开发步骤

### 阶段 B1：网络层

1. 实现 `HttpResponse`。
2. 实现 `CookieHttpClient`。
3. 实现 Header 工具。
4. 实现超时和一次重试。
5. 写 Cookie 单测。

### 阶段 B2：统一认证

1. 实现 `ScuAuthService.fetchCaptcha()`。
2. 实现 SM2 加密工具。
3. 实现 `login()`。
4. 实现 token 保存和恢复。
5. 实现 `bindSession()`。
6. 提供 AuthViewModel 给 A。

### 阶段 B3：教务认证和课表 API

1. 实现 ZhjwAuthService。
2. 实现 `fetchCurrentWeek()`。
3. 实现 `fetchSemesters()`。
4. 实现 `fetchJwxtSchedule()`。
5. 用 mock 或真实账号联调 C 的在线导入。

### 阶段 B4：考表 API

1. 实现 `fetchExamPlan()`。
2. 复刻 Flutter 正则解析。
3. 交给 D 排序和页面展示。
4. 写 HTML parser 单测。

### 阶段 B5：成绩 API

1. 实现 `fetchSchemeScores()`。
2. 实现 `fetchPassingScores()`。
3. 写 callback URL 提取测试。
4. 与 D 联调方案成绩、及格成绩、自定义统计。

## 14. 验收标准

人员 B 完成时必须满足：

- 登录页可以获取验证码。
- 用户手动输入验证码后可以登录。
- token 可以恢复，1 小时 TTL 生效。
- 教务 SSO 可获取课程学期列表。
- 课表 JSON 可返回给 C。
- 当前教学周可返回给 C。
- 考表可返回结构化列表给 D。
- 方案成绩 JSON 可返回给 D。
- 及格成绩 JSON 可返回给 D。
- 教务会话过期后自动重试一次。
- 自动重试失败后 UI 收到未登录状态。
- 无 OCR 依赖。
- 无体测 SSO/API 作为第一阶段交付项。

## 15. 文档间冲突检查结果

人员 B 与其他人员边界：

- B 只做认证、网络、教务 API 和远端必要解析。
- C 负责课表领域解析和本地保存。
- D 负责查询页面状态、缓存、成绩业务模型、排序和展示。
- A 负责登录页 UI，不实现登录算法。
- E 负责测试打包，不实现 API。

明确避免的冲突：

- `fetchJwxtSchedule()` 返回原始 JSON，不直接写入课表数据库。
- `fetchExamPlan()` 返回 DTO，不直接操作 D 的页面状态。
- `fetchSchemeScores()` 和 `fetchPassingScores()` 返回原始 JSON，不直接计算 GPA。
- 校历公开页抓取由 D 实现，B 只提供普通 HTTP 能力。
- 体测相关认证与 API 不在第一阶段实现。

需要持续协调的点：

- API DTO 字段名变更前通知 C/D/E。
- AuthViewModel 属性名变更前通知 A。
- 错误类型新增时通知 A/D/E 更新显示和测试。
