# SCU_Nexus 人员 D 查询模块

这是按人员 D 任务书新建的 Qt 6 + CMake + C++ + Qt Widgets 工程骨架，覆盖第一阶段查询类功能：

- 校历查询
- 考表查询
- 教务成绩

当前工程使用 `MockZhjwApiService` 模拟人员 B 的教务接口，便于 D 模块独立开发和展示。后续接入真实接口时，只需要替换 `src/services/zhjw/MockZhjwApiService.*` 的调用来源。

## 目录

```text
src/
  common/                 QueryState
  models/                 校历、考表、成绩模型和统计逻辑
  repositories/           QueryCacheRepository(SQLite)
  services/calendar/      校历公开页面抓取与解析
  services/grades/        成绩统计服务
  services/zhjw/          人员 B API mock 适配
  viewmodels/             三个页面 ViewModel
  widgets/                纯 C++ Qt Widgets 页面
tests/                    Qt Test 单元测试
```

## 构建

需要安装 Qt 6、CMake 和 C++ 编译器。

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvcxxxx_64"
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

## 已实现

- 统一查询状态：`Idle`、`Loading`、`Loaded`、`Empty`、`Error`、`LoginRequired`。
- SQLite 查询缓存表：`query_cache(cache_key, payload_json, updated_at)`。
- 校历列表/详情 HTML 解析、图片 URL 解析、编码 fallback。
- 考表 `isPast` 判断、时间排序、缓存优先展示。
- 成绩 JSON 字段映射、GPA、必修 GPA、加权均分、学分、学期分组、自定义统计。
- 纯 C++ Qt Widgets 页面：主窗口、校历、考表、成绩三 Tab。

## 接入边界

- 人员 A：可替换 `src/widgets/` 下的状态组件和主壳路由。
- 人员 B：用真实 `ZhjwApiService` 替换 mock 的 `fetchExamPlan()`、`fetchSchemeScores()`、`fetchPassingScores()` 和登录状态。
- 人员 E：可直接扩展 `tests/tst_d_module.cpp` 的 fixture 和回归测试。

## 不包含

- SCU 统一认证、Cookie、教务 SSO。
- 课表数据库。
- 体测查询。
- Windows 安装包打包。
