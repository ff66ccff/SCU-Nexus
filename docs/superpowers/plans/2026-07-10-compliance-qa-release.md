# QA, Packaging, and Compliance Evidence Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 建立能证明项目满足文档的自动化门禁、Windows 发布流程和需求证据矩阵，完成 REQ-011～REQ-013 及 15 项正式验收的收口。

**Architecture:** CMake/CTest 统一驱动单元测试和 QML smoke；独立 Python 脚本检查分层与敏感逻辑；PowerShell 脚本构建和验证自包含 Windows 包；Markdown 矩阵将每条需求映射到实现、自动化和人工证据。

**Tech Stack:** C++20、Qt 6.11、CMake/CTest 3.30、Python 3、PowerShell 5.1+、MinGW 13.1、windeployqt。

## Global Constraints

- 本计划在核心、课表和查询三个计划完成后执行。
- Debug 与 Release 均必须从当前源码重新构建，不能引用旧输出作为证据。
- 发布包不得依赖开发机源码路径或预设 PATH，不得包含真实账号缓存和测试凭据。
- 真实校园账号与干净 Windows 实机只可标记 `人工待验`，不得伪造通过。
- REQ-013 只验证扩展边界；不实现任何 P2 功能。
- 所有 QA 状态必须来自代码、命令输出或人工记录，不能凭文件存在判定通过。
- 不修改或提交由行尾差异造成的既有工作树改动。

---

## File Map

- `SCU_Nexus/tests/fixtures/*`: 虚构、脱敏、可复现的协议样本。
- `SCU_Nexus/tests/qml/tst_query_state_pane.qml`: 通用状态组件 smoke。
- `SCU_Nexus/tests/test_app_foundation.cpp`: Router、设置、缓存和启动测试。
- `SCU_Nexus/scripts/conflict_check.py`: QML 敏感逻辑、跨层依赖、重复注册和占位检查。
- `SCU_Nexus/scripts/package_windows.ps1`: Release 构建、windeployqt、运行时复制和校验和。
- `SCU_Nexus/scripts/verify_package.ps1`: 隔离 PATH/AppData 的包体验证和启动 smoke。
- `SCU_Nexus/main.cpp`: `--smoke-test` 自动退出模式。
- `SCU_Nexus/CMakeLists.txt`: CTest、lint/install 和可配置 OpenSSL。
- `SCU_Nexus/docs/qa/requirements_compliance_matrix.md`: 唯一需求状态入口。
- `SCU_Nexus/docs/qa/manual_acceptance_checklist.md`: 真实账号/UI/干净机记录。
- `SCU_Nexus/docs/qa/release_checklist.md`: 发布门禁与签字栏。
- `SCU_Nexus/docs/qa/optional_modules_research.md`: P2 边界清单。
- `README.md`: 构建、测试、运行、缓存、打包和隐私说明。

### Task 1: Consolidate fictional protocol fixtures

**Files:**
- Create: `SCU_Nexus/tests/fixtures/auth_captcha.json`
- Create: `SCU_Nexus/tests/fixtures/exam_plan.html`
- Create: `SCU_Nexus/tests/fixtures/exam_plan_empty.html`
- Create: `SCU_Nexus/tests/fixtures/jwxt_schedule.json`
- Verify: `SCU_Nexus/tests/fixtures/calendar_entries.html`
- Verify: `SCU_Nexus/tests/fixtures/calendar_empty.html`
- Verify: `SCU_Nexus/tests/fixtures/scheme_scores.json`
- Verify: `SCU_Nexus/tests/fixtures/passing_scores.json`
- Modify: `SCU_Nexus/tests/test_person_b_foundation.cpp`
- Modify: `SCU_Nexus/tests/test_schedule.cpp`

**Interfaces:**
- Produces: stable fixture files loadable with `QFINDTESTDATA` and containing no sensitive data.

- [ ] **Step 1: Add a fixture-loading helper to each test executable**

Use the same explicit helper:

```cpp
QByteArray loadFixture(const QString &name)
{
    const QString path = QFINDTESTDATA(QStringLiteral("fixtures/") + name);
    QFile file(path);
    if (path.isEmpty() || !file.open(QIODevice::ReadOnly)) {
        qFatal("fixture unavailable: %s", qPrintable(name));
    }
    return file.readAll();
}
```

- [ ] **Step 2: Create minimal valid and empty samples**

Use fictional identity `202600000001`, token text `fixture-token-never-valid`, courses `示例课程甲/乙`, and non-real scores. Every JSON file must parse with `QJsonDocument`; every HTML file contains only the minimum structure required by the parser and an explicit UTF-8 meta tag.

- [ ] **Step 3: Replace large inline parser payloads**

Parser tests read the fixture while small one-line boundary inputs remain inline. Add a privacy assertion that extracts every `\\b\\d{12}\\b` value from all fixtures and verifies the resulting set is a subset of `{202600000001}`.

- [ ] **Step 4: Run B, C and D tests**

Expected: all three existing module executables plus `app_foundation_tests` exit 0.

- [ ] **Step 5: Commit**

```bash
git add SCU_Nexus/tests/fixtures SCU_Nexus/tests/test_person_b_foundation.cpp SCU_Nexus/tests/test_schedule.cpp SCU_Nexus/tests/test_person_d_queries.cpp
git commit -m "test: centralize fictional campus protocol fixtures"
```

### Task 2: Add application and QML smoke coverage

**Files:**
- Modify: `SCU_Nexus/tests/test_app_foundation.cpp`
- Create: `SCU_Nexus/tests/qml/tst_query_state_pane.qml`
- Modify: `SCU_Nexus/CMakeLists.txt`

**Interfaces:**
- Produces: CTest entries `app_foundation_tests` and `qml_smoke_tests`.

- [ ] **Step 1: Test Router's documented navigation**

Add Router sources to the foundation target and assert the exact route sequence:

```cpp
void routerNavigatesAllRequiredPages()
{
    Router router;
    const QStringList routes{"Schedule", "AcademicCalendar", "ExamPlan", "Grades", "Settings", "Login"};
    for (const QString &route : routes) {
        router.navigate(route);
        QCOMPARE(router.currentRoute(), route);
        QVERIFY(!router.routeTitle().isEmpty());
    }
    QVERIFY(router.canGoBack());
    router.goBack();
    QCOMPARE(router.currentRoute(), QStringLiteral("Settings"));
}
```

Add `ThemeManager.{h,cpp}` to the foundation target. Point `QSettings::IniFormat` user scope to a `QTemporaryDir`, set mode to `dark`, destroy the manager, construct another manager, and assert `mode()` remains `dark`. Set `QCoreApplication::applicationVersion("0.1.0")` and assert the test `AppController::appVersion()` returns `0.1.0`. These two assertions are the automated evidence for REQ-012.

- [ ] **Step 2: Create a QML state-pane smoke test**

Create:

```qml
import QtQuick
import QtTest
import "../../qml/components/query"

TestCase {
    name: "QueryStatePane"
    width: 640
    height: 480

    Component { id: paneComponent; QueryStatePane { width: 640; height: 480 } }

    function test_visibility_contract() {
        const pane = createTemporaryObject(paneComponent, this)
        verify(pane !== null)
        pane.queryState = 1; compare(pane.visible, true)
        pane.queryState = 2; compare(pane.visible, false)
        pane.queryState = 3; compare(pane.visible, false)
        pane.queryState = 4; compare(pane.visible, true)
        pane.queryState = 5; compare(pane.visible, true)
        pane.queryState = 6; compare(pane.visible, true)
    }
}
```

- [ ] **Step 3: Register qmltestrunner without a machine-specific path**

Find `qmltestrunner` from the active Qt host bin directory and fail CMake configuration if absent when `BUILD_TESTING` is on. Register:

```cmake
add_test(NAME qml_smoke_tests
    COMMAND "${QMLTESTRUNNER_EXECUTABLE}"
        -input "${CMAKE_CURRENT_SOURCE_DIR}/tests/qml"
        -import "${CMAKE_CURRENT_SOURCE_DIR}/qml")
set_tests_properties(qml_smoke_tests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
```

- [ ] **Step 4: Run CTest with output on failure**

Expected: Router and state-pane tests pass in an offscreen environment.

- [ ] **Step 5: Commit**

```bash
git add SCU_Nexus/tests/test_app_foundation.cpp SCU_Nexus/tests/qml/tst_query_state_pane.qml SCU_Nexus/CMakeLists.txt
git commit -m "test: add application navigation and QML smoke gates"
```

### Task 3: Implement the static conflict and architecture check

**Files:**
- Create: `SCU_Nexus/scripts/conflict_check.py`
- Modify: `SCU_Nexus/CMakeLists.txt`

**Interfaces:**
- Produces: deterministic exit 0/1 script and CTest entry `architecture_conflict_check`.

- [ ] **Step 1: Create the checker with explicit rules**

The script must use `pathlib` and implement these exact scans:

```python
FORBIDDEN_QML = {
    "business URL": re.compile(r"https?://", re.I),
    "SQL": re.compile(r"\b(?:SELECT|INSERT|UPDATE|DELETE|CREATE\s+TABLE)\b", re.I),
    "Cookie": re.compile(r"\b(?:Set-Cookie|CookieJar|cookieHeader)\b", re.I),
    "SM2": re.compile(r"\bSM2\b|sm2_key|C1C2C3", re.I),
}
FORBIDDEN_SOURCE_MARKERS = re.compile(r"\b(?:TBD|TODO|FIXME)\b|接口已预留|PagePlaceholder")
LAYER_RULES = {
    "models": ("viewmodels/", "services/", "repositories/", "qml/"),
    "repositories": ("viewmodels/", "qml/"),
    "services": ("viewmodels/", "qml/"),
}
```

Resolve the repository root from `Path(__file__).resolve().parents[1]`. Scan `qml/**/*.qml` and `src/**/*.{h,cpp}`. Print `relative_path:line: rule: excerpt` for every finding. Parse `qmlRegisterType`, `qmlRegisterSingletonType`, and `qmlRegisterUncreatableMetaObject` calls and report duplicate `(URI, major, minor, QML name)` tuples. Exit 1 if any finding exists, otherwise print a count summary and exit 0.

- [ ] **Step 2: Run it and classify every initial finding**

Run:

```bash
python3 SCU_Nexus/scripts/conflict_check.py
```

Fix real violations in their owning task. If a benign string is unavoidable, add a narrow `(relative path, rule, exact line text)` allow-list entry with a comment; do not disable a rule globally.

- [ ] **Step 3: Register as a test**

Use `find_package(Python3 REQUIRED COMPONENTS Interpreter)` and `add_test` with `${Python3_EXECUTABLE}`.

- [ ] **Step 4: Verify pass and commit**

```bash
git add SCU_Nexus/scripts/conflict_check.py SCU_Nexus/CMakeLists.txt
git commit -m "test: enforce QML and architecture boundaries"
```

### Task 4: Parameterize OpenSSL and add install rules

**Files:**
- Modify: `SCU_Nexus/CMakeLists.txt:1-335`

**Interfaces:**
- Produces: cache variables `SCU_NEXUS_OPENSSL_ROOT` and `SCU_NEXUS_OPENSSL_RUNTIME`; installable executable.

- [ ] **Step 1: Verify configuration currently depends on `D:/QT`**

Configure a new build while passing a different OpenSSL root. Expected before the change: CMake still uses the hard-coded path.

- [ ] **Step 2: Replace hard-coded values with cache variables**

Use:

```cmake
set(SCU_NEXUS_OPENSSL_ROOT "" CACHE PATH "OpenSSL root containing include, lib and bin")
set(SCU_NEXUS_OPENSSL_RUNTIME "" CACHE FILEPATH "OpenSSL crypto runtime DLL copied beside the app")
if(NOT SCU_NEXUS_OPENSSL_ROOT)
    message(FATAL_ERROR "Set SCU_NEXUS_OPENSSL_ROOT to the MinGW-compatible OpenSSL root")
endif()
find_path(SCU_NEXUS_OPENSSL_INCLUDE_DIR openssl/evp.h REQUIRED
    HINTS "${SCU_NEXUS_OPENSSL_ROOT}/include")
find_library(SCU_NEXUS_OPENSSL_CRYPTO_LIBRARY NAMES crypto libcrypto.dll.a REQUIRED
    HINTS "${SCU_NEXUS_OPENSSL_ROOT}/lib")
```

Link/include these resolved variables. Execute the post-build copy only when the runtime variable names an existing file.

- [ ] **Step 3: Add install rules**

```cmake
include(GNUInstallDirs)
install(TARGETS SCU_Nexus RUNTIME DESTINATION . BUNDLE DESTINATION .)
if(SCU_NEXUS_OPENSSL_RUNTIME)
    install(FILES "${SCU_NEXUS_OPENSSL_RUNTIME}" DESTINATION .)
endif()
```

- [ ] **Step 4: Configure Debug and Release with explicit values**

Expected: both configurations succeed and CMake cache contains no forced `D:/QT` OpenSSL assignment other than the caller-provided value.

- [ ] **Step 5: Commit**

```bash
git add SCU_Nexus/CMakeLists.txt
git commit -m "build: parameterize OpenSSL runtime inputs"
```

### Task 5: Add deterministic packaged-app smoke mode

**Files:**
- Modify: `SCU_Nexus/main.cpp`
- Test: `SCU_Nexus/tests/test_app_foundation.cpp`

**Interfaces:**
- Produces: command-line `--smoke-test` that exits 0 only after the root QML object is created.

- [ ] **Step 1: Extract and test option detection**

Add a pure helper `bool smokeTestRequested(const QStringList &arguments)` in a small `src/app/AppOptions.{h,cpp}` and test false for normal arguments and true only for an exact `--smoke-test` token.

- [ ] **Step 2: Implement post-load exit behavior**

After connecting `objectCreated`, if smoke mode is active, start a single-shot 1500 ms timer only when `obj` is non-null and the root URL matches. The timer calls `QCoreApplication::quit()`. Object creation failure retains exit `-1`; add a 10-second fail-safe that exits `2` so package verification cannot hang.

- [ ] **Step 3: Build and invoke the development executable**

Run `SCU_Nexus.exe --smoke-test` with an isolated temporary `APPDATA`. Expected: exit 0 within 10 seconds.

- [ ] **Step 4: Commit**

```bash
git add SCU_Nexus/src/app/AppOptions.h SCU_Nexus/src/app/AppOptions.cpp SCU_Nexus/main.cpp SCU_Nexus/tests/test_app_foundation.cpp SCU_Nexus/CMakeLists.txt
git commit -m "test: add packaged application smoke mode"
```

### Task 6: Create the Windows package builder

**Files:**
- Create: `SCU_Nexus/scripts/package_windows.ps1`

**Interfaces:**
- Consumes: source/build/output paths, Qt bin, OpenSSL root, CMake executable.
- Produces: clean `dist/SCU_Nexus/`, `manifest.json`, `SHA256SUMS.txt`.

- [ ] **Step 1: Implement strict parameters and preflight**

Use a `[CmdletBinding()]` script with parameters `SourceDir`, `BuildDir`, `OutputDir`, `QtBin`, `OpenSslRoot`, and `CMakeExe`. Set `$ErrorActionPreference = 'Stop'`; resolve all paths; require `cmake.exe`, `windeployqt.exe`, `SCU_Nexus.exe`, and `libcrypto-1_1-x64.dll` before packaging.

- [ ] **Step 2: Configure and build Release**

Invoke CMake with:

```powershell
& $CMakeExe -S $SourceDir -B $BuildDir `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_PREFIX_PATH=(Split-Path $QtBin -Parent) `
  -DSCU_NEXUS_OPENSSL_ROOT=$OpenSslRoot `
  -DSCU_NEXUS_OPENSSL_RUNTIME=(Join-Path $OpenSslRoot 'bin\libcrypto-1_1-x64.dll')
if ($LASTEXITCODE -ne 0) { throw 'Release configure failed' }
& $CMakeExe --build $BuildDir --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw 'Release build failed' }
```

- [ ] **Step 3: Deploy into a clean output directory**

Delete only the caller-provided output directory after verifying it is not a drive root or the source directory. Copy the executable and OpenSSL runtime, then run:

```powershell
& (Join-Path $QtBin 'windeployqt.exe') --release --compiler-runtime `
  --qmldir (Join-Path $SourceDir 'qml') $TargetExe
if ($LASTEXITCODE -ne 0) { throw 'windeployqt failed' }
```

- [ ] **Step 4: Generate manifest and hashes**

Enumerate every file relative to the package root, record relative path, byte length and SHA-256 in `manifest.json`, and write `hash *relative/path` lines sorted ordinally to `SHA256SUMS.txt`. Exclude neither DLL nor QML files.

- [ ] **Step 5: Execute against a temporary release directory and commit**

Expected: script exits 0 and creates an executable, Qt runtime DLLs, platform and SQL plugins, QML modules, OpenSSL DLL, manifest and hashes.

```bash
git add SCU_Nexus/scripts/package_windows.ps1
git commit -m "build: add reproducible Windows package script"
```

### Task 7: Verify the package independently of developer PATH

**Files:**
- Create: `SCU_Nexus/scripts/verify_package.ps1`

**Interfaces:**
- Consumes: package directory.
- Produces: nonzero exit on missing/tampered runtime or failed smoke; JSON verification report.

- [ ] **Step 1: Validate required structure and hashes**

Require `SCU_Nexus.exe`, `Qt6Core.dll`, `Qt6Gui.dll`, `Qt6Qml.dll`, `Qt6Quick.dll`, `platforms/qwindows.dll`, `sqldrivers/qsqlite.dll`, `libcrypto-1_1-x64.dll`, `manifest.json`, and `SHA256SUMS.txt`. Recompute each manifest hash and fail on missing, extra mismatch, or changed length.

- [ ] **Step 2: Launch in an isolated environment**

Save the current process environment, then set `PATH` to the package directory plus Windows system directories only; set `APPDATA` and `LOCALAPPDATA` to newly created temporary directories. Start `SCU_Nexus.exe --smoke-test`, wait at most 15 seconds, kill and fail on timeout, and require exit code 0.

- [ ] **Step 3: Write the report**

Write `verification-report.json` containing UTC time, package path, file count, hash result, smoke exit code, and `cleanWindowsVerified: false`. The last field stays false until an operator records a separate clean-machine check.

- [ ] **Step 4: Verify positive and negative cases**

Expected: untouched package passes. Temporarily rename `platforms/qwindows.dll` and expect a nonzero exit naming the missing file; restore it and verify pass again.

- [ ] **Step 5: Commit**

```bash
git add SCU_Nexus/scripts/verify_package.ps1
git commit -m "build: verify self-contained Windows packages"
```

### Task 8: Create the requirement and manual-evidence documents

**Files:**
- Create: `SCU_Nexus/docs/qa/requirements_compliance_matrix.md`
- Create: `SCU_Nexus/docs/qa/manual_acceptance_checklist.md`
- Create: `SCU_Nexus/docs/qa/release_checklist.md`
- Create: `SCU_Nexus/docs/qa/optional_modules_research.md`

**Interfaces:**
- Produces: one evidence row per REQ-001～REQ-013 and per V4/V5 acceptance item 1～15.

- [ ] **Step 1: Create the matrix with controlled statuses**

Define only `已满足`, `补齐后满足`, `人工待验`, and `明确不适用`. Each row includes source section, observable behavior, implementation file/symbol, test/command, manual checklist ID, current status, and notes. Add all 13 REQ rows and all 15 acceptance rows; no merged catch-all rows.

- [ ] **Step 2: Populate evidence from current code and fresh results**

Use absolute requirement IDs but repository-relative file links. Mark a row `已满足` only after its named automated command passes. Mark real login/import/exam/grade and clean-machine package rows `人工待验`. REQ-013 is `明确不适用` with evidence from `conflict_check.py` and the optional-module document.

- [ ] **Step 3: Create the manual checklist**

Give every item a stable ID `MAN-A01` onward, checkbox, prerequisites, exact action, expected result, actual result, environment, date and tester. Cover navigation, window restore/minimum, login/logout/expiry, course CRUD/conflict/multi-schedule, real import, calendar, exam, grades, cache refresh retention/clear, theme/version, access control and clean Windows launch.

- [ ] **Step 4: Create release and P2-boundary documents**

The release checklist records commit, build type, four test targets, QML lint, conflict check, package/verify commands, artifact hash, privacy scan, known external pending items and sign-off. The optional-module document inventories reference-only P2 capabilities, records why each is excluded, and names the extension boundary that remains available; it contains no implementation promise.

- [ ] **Step 5: Run a matrix integrity scan and commit**

Use `rg` to confirm REQ-001 through REQ-013 and acceptance 1 through 15 each appear exactly once in their primary tables; scan for unsupported `已满足` rows with blank evidence cells.

```bash
git add SCU_Nexus/docs/qa
git commit -m "docs: add compliance and release evidence matrix"
```

### Task 9: Expand the project README

**Files:**
- Modify: `README.md`

**Interfaces:**
- Produces: a reproducible operator entry point for build, test, run, data, package and privacy behavior.

- [ ] **Step 1: Document prerequisites and exact configuration**

List Windows 10/11, Qt 6.11.1 MinGW 64-bit modules, CMake 3.30+, MinGW 13.1, Python 3, PowerShell 5.1+, and the compatible OpenSSL root/runtime variables. Do not embed the author's local absolute path as a required value.

- [ ] **Step 2: Document commands and data locations**

Include copyable Debug configure/build, `ctest --output-on-failure`, `all_qmllint`, conflict check, package and verify commands. Explain `QStandardPaths::AppDataLocation`, query-cache and schedule database files, Settings clear actions, and that clearing local data cannot be undone.

- [ ] **Step 3: Document privacy and external verification**

State that passwords are not stored, logs are redacted, fixtures are fictional, token storage is a documented technical debt, and real campus/clean Windows checks must be recorded in the manual checklist.

- [ ] **Step 4: Verify every referenced path exists and commit**

```bash
git add README.md
git commit -m "docs: document build test package and privacy workflows"
```

### Task 10: Execute the completion audit

**Files:**
- Modify: `SCU_Nexus/docs/qa/requirements_compliance_matrix.md`
- Modify: `SCU_Nexus/docs/qa/release_checklist.md`

**Interfaces:**
- Produces: fresh, requirement-by-requirement evidence for the original objective.

- [ ] **Step 1: Configure a fresh Debug build**

Use a new build directory and explicit Qt/OpenSSL inputs. Expected: configuration exits 0 without relying on old cache values.

- [ ] **Step 2: Build and run every automated gate**

Run the all-target build, CTest with `--output-on-failure`, each test executable if CTest cannot supply its DLL PATH, `all_qmllint`, and `conflict_check.py`. Capture command, UTC/local time, exit code and test totals.

- [ ] **Step 3: Configure and build a fresh Release**

Use a second new build directory. Expected: Release build exits 0 and produces `SCU_Nexus.exe` with no debug-only runtime dependency.

- [ ] **Step 4: Package and verify**

Run `package_windows.ps1`, then `verify_package.ps1`. Expected: hashes and isolated smoke pass. Keep clean-machine validation false until externally executed.

- [ ] **Step 5: Perform security and repository hygiene scans**

Use `rg` to search tracked source, tests and docs for password/token/Cookie values, real-looking 12-digit IDs, business URLs in QML, SQL in QML, `TODO/TBD/FIXME`, placeholder strings, build directories and cache databases. Classify and remove every true positive; fixture sentinel values remain explicitly documented.

- [ ] **Step 6: Audit all 28 primary rows against authoritative evidence**

For each REQ and acceptance row, open the named implementation, inspect the named test's assertion scope, and match it to the observable requirement. Downgrade weak or indirect evidence instead of inferring completion. Ensure external items remain `人工待验` and are excluded from automated pass counts.

- [ ] **Step 7: Record final results and commit evidence**

```bash
git add SCU_Nexus/docs/qa/requirements_compliance_matrix.md SCU_Nexus/docs/qa/release_checklist.md
git commit -m "docs: record final compliance verification evidence"
```

- [ ] **Step 8: Completion decision**

Declare the requested implementation complete only if all code/package deliverables are present, every automated gate passes from fresh builds, and every formal row has strong evidence or is explicitly an external `人工待验`/P2 `明确不适用`. Report the external pending items separately; do not count them as passed.
