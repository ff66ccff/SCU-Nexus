# 教室查询数据来源标注 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在教室查询页标题下方显示“数据来自”及指定教务系统 URL，并支持点击后在外部浏览器打开。

**Architecture:** 直接复用现有 `SourceAttribution` QML 组件，使教室查询页与课表页保持相同的文案、排版和点击行为。通过现有 UI 源码契约测试锁定精确 URL 和组件接入，不新增组件或业务状态。

**Tech Stack:** Qt 6、QML、Qt Test、CMake/CTest

## Global Constraints

- 三个页面统一显示“数据来自”，不得修改 `SourceAttribution` 的现有前缀。
- 教室查询来源 URL 必须精确为 `http://zhjw.scu.edu.cn/student/teachingResources/classroomCurriculum/index`。
- 来源说明位于教室查询标题栏之后、查询操作之前，并在校区、教学楼和教室三个层级始终显示。
- 点击行为继续由 `SourceAttribution` 中的 `Qt.openUrlExternally(sourceUrl)` 提供。
- 不新增依赖，不修改教室查询数据流和状态管理。

## File Structure

- `SCU_Nexus/tests/test_ui_contracts.cpp`：扩展现有来源标注契约，验证教室页面使用精确 URL。
- `SCU_Nexus/qml/pages/classroom/ClassroomPage.qml`：在标题栏后复用 `SourceAttribution`。

---

### Task 1: 为教室查询接入可点击来源标注

**Files:**
- Modify: `SCU_Nexus/tests/test_ui_contracts.cpp:98-115`
- Modify: `SCU_Nexus/qml/pages/classroom/ClassroomPage.qml:33-66`

**Interfaces:**
- Consumes: `SourceAttribution { property string sourceUrl }`，组件内部点击调用 `Qt.openUrlExternally(sourceUrl)`。
- Produces: 教室查询页中的全宽来源说明，URL 为 `http://zhjw.scu.edu.cn/student/teachingResources/classroomCurriculum/index`。

- [ ] **Step 1: 写入失败的 UI 契约测试**

在 `sourceAttributionsExposeExactClickableUrls()` 中读取教室页面，并新增精确 URL 断言：

```cpp
const QString classroom = readUtf8(
    QStringLiteral("qml/pages/classroom/ClassroomPage.qml"));

QVERIFY(classroom.contains(QStringLiteral(
    "http://zhjw.scu.edu.cn/student/teachingResources/classroomCurriculum/index")));
```

- [ ] **Step 2: 运行测试并确认 RED**

从 `SCU_Nexus` 目录运行：

```bash
cmake --preset dev
cmake --build ../out/build/dev --target ui_contract_tests
ctest --test-dir ../out/build/dev -R '^ui_contract_tests$' --output-on-failure
```

Expected: `ui_contract_tests` 失败，失败点为教室页面尚未包含精确来源 URL。

- [ ] **Step 3: 写入最小 QML 实现**

在 `ClassroomPage.qml` 的标题 `RowLayout` 之后、仅教室层显示的查询操作 `RowLayout` 之前加入：

```qml
SourceAttribution {
    Layout.fillWidth: true
    sourceUrl: "http://zhjw.scu.edu.cn/student/teachingResources/classroomCurriculum/index"
}
```

- [ ] **Step 4: 运行针对性测试并确认 GREEN**

从 `SCU_Nexus` 目录运行：

```bash
cmake --build ../out/build/dev --target ui_contract_tests
ctest --test-dir ../out/build/dev -R '^ui_contract_tests$' --output-on-failure
```

Expected: `100% tests passed, 0 tests failed out of 1`。

- [ ] **Step 5: 运行 QML/应用构建回归验证**

从 `SCU_Nexus` 目录运行：

```bash
cmake --build ../out/build/dev --target SCU_Nexus
ctest --test-dir ../out/build/dev --output-on-failure
```

Expected: `SCU_Nexus` 构建成功，全部 CTest 测试通过。

- [ ] **Step 6: 提交实现**

```bash
git add SCU_Nexus/tests/test_ui_contracts.cpp \
        SCU_Nexus/qml/pages/classroom/ClassroomPage.qml \
        docs/superpowers/plans/2026-07-15-classroom-source-attribution.md
git commit -m "feat: show classroom data source"
```
