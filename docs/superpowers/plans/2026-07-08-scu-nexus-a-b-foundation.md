# SCU Nexus A/B Foundation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bring the Qt app foundation in line with Person A's app-framework/UI document and add Person B's auth/network/API foundation with offline-verifiable behavior.

**Architecture:** Person A owns app shell, route state, theme, reusable QML components, login/settings shells, and placeholders only. Person B owns `src/core` and `src/services` code for network, auth state, DTOs, parser utilities, and service interfaces; real network code stays outside QML and parser behavior is covered by tests using static input.

**Tech Stack:** Qt 6 Core/Gui/Qml/Quick/QuickControls2/Network/Sql/Test, C++20, QML resource module `SCU_Nexus`, Qt Test for unit tests.

## Global Constraints

- Person A work must stay inside `SCU_Nexus/docs/team/01_person_A_app_framework_ui.md` boundaries: no business HTTP URL, SQL, SM2 encryption, or Cookie handling in QML.
- Stage one navigation has exactly four business entries plus settings: Schedule, AcademicCalendar, ExamPlan, Grades, Settings.
- Main window dimensions are `width: 1100`, `height: 760`, `minimumWidth: 900`, `minimumHeight: 620`.
- Schedule and AcademicCalendar do not require login; ExamPlan and Grades require login.
- Person B first stage excludes OCR and fitness/physical-test APIs.
- Tests must not hit the real campus network; use static fake responses.

---

## File Structure

- Modify `SCU_Nexus/CMakeLists.txt`: add `CMAKE_AUTOMOC`, `qt_add_qml_module`, B source files, and Qt Test executable.
- Modify `SCU_Nexus/main.cpp`: initialize app metadata, Quick Controls style, AppController, Router, ThemeManager, ToastManager, and load `qrc:/SCU_Nexus/qml/App.qml`.
- Modify `SCU_Nexus/src/app/*`: strengthen AppController, Router, ThemeManager, ToastManager APIs.
- Create `SCU_Nexus/qml/styles/Theme.qml`: centralized color/metric helper singleton.
- Modify `SCU_Nexus/qml/*.qml` and `SCU_Nexus/qml/components/*.qml`: modernized shell, reusable state components, login/settings shells.
- Create `SCU_Nexus/qml/components/AppDialog.qml`, `DataTable.qml`, `FormRow.qml`, `IconButton.qml`, `ModuleHeader.qml`, `SearchBar.qml`, `SectionHeader.qml`, `ToastHost.qml`.
- Create `SCU_Nexus/src/core/network/*`: Cookie HTTP DTOs and cookie jar utilities.
- Create `SCU_Nexus/src/services/auth/*`: Auth DTOs, errors, AuthViewModel stub boundary.
- Create `SCU_Nexus/src/services/api/*`: ZHJW parser utilities and DTOs.
- Create `SCU_Nexus/tests/test_person_b_foundation.cpp`: offline unit tests for Person B parser/cookie/session behavior.

### Task 1: Person B Offline Foundation Tests

**Files:**
- Create: `SCU_Nexus/tests/test_person_b_foundation.cpp`
- Modify: `SCU_Nexus/CMakeLists.txt`

**Interfaces:**
- Consumes: none.
- Produces: test expectations for `CookieJar`, `ZhjwParsers::isSessionExpired`, `parseCurrentWeek`, `parseSemesters`, `parseExamPlan`, `extractSchemeScoresCallback`, and `extractPassingScoresCallback`.

- [ ] **Step 1: Write the failing test**

Add tests that include headers that do not exist yet:

```cpp
#include <QtTest/QtTest>
#include "src/core/network/CookieJar.h"
#include "src/services/api/ZhjwParsers.h"

class PersonBFoundationTest : public QObject
{
    Q_OBJECT
private slots:
    void cookieJarMatchesExactAndParentDomain();
    void cookieJarRejectsUnrelatedDomain();
    void cookieJarParsesCommaSeparatedSetCookie();
    void zhjwParsersExtractRequiredValues();
    void zhjwParsersDetectExpiredSessions();
};
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug --target person_b_foundation_tests`
Expected: FAIL because the test target and B headers are missing.

- [ ] **Step 3: Implement minimal CMake test target**

Add a Qt Test executable named `person_b_foundation_tests` linked with Qt6::Core, Qt6::Network, and Qt6::Test.

- [ ] **Step 4: Run test to verify it still fails for missing production code**

Run: `cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug --target person_b_foundation_tests`
Expected: FAIL because `CookieJar.h` and `ZhjwParsers.h` are still missing.

### Task 2: Person B Cookie And Parser Utilities

**Files:**
- Create: `SCU_Nexus/src/core/network/CookieJar.h`
- Create: `SCU_Nexus/src/core/network/CookieJar.cpp`
- Create: `SCU_Nexus/src/core/network/HttpResponse.h`
- Create: `SCU_Nexus/src/core/network/NetworkError.h`
- Create: `SCU_Nexus/src/services/api/ApiDtos.h`
- Create: `SCU_Nexus/src/services/api/ZhjwParsers.h`
- Create: `SCU_Nexus/src/services/api/ZhjwParsers.cpp`
- Modify: `SCU_Nexus/CMakeLists.txt`
- Test: `SCU_Nexus/tests/test_person_b_foundation.cpp`

**Interfaces:**
- Produces: `CookieJar::storeFromSetCookie(QUrl, QList<QByteArray>)`, `CookieJar::cookieHeader(QUrl)`, `CookieJar::clear()`.
- Produces: `ZhjwParsers::isSessionExpired`, `parseCurrentWeek`, `parseSemesters`, `parseExamPlan`, `extractSchemeScoresCallback`, `extractPassingScoresCallback`.

- [ ] **Step 1: Run failing tests**

Run: `cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug --target person_b_foundation_tests && build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug/person_b_foundation_tests`
Expected: FAIL before production code exists.

- [ ] **Step 2: Implement cookie jar and parser utilities**

Implement host-scoped cookie storage, parent-domain matching, safe comma split for `Set-Cookie`, current-week regex `第(\\d+)周`, semester option parsing, exam card extraction, callback URL extraction, and session-expired heuristics.

- [ ] **Step 3: Run tests to verify they pass**

Run: `cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug --target person_b_foundation_tests && build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug/person_b_foundation_tests`
Expected: PASS.

### Task 3: Person A App Shell And UI Components

**Files:**
- Modify: `SCU_Nexus/src/app/AppController.h`
- Modify: `SCU_Nexus/src/app/AppController.cpp`
- Modify: `SCU_Nexus/src/app/Router.h`
- Modify: `SCU_Nexus/src/app/Router.cpp`
- Modify: `SCU_Nexus/src/app/ThemeManager.h`
- Modify: `SCU_Nexus/src/app/ThemeManager.cpp`
- Modify: `SCU_Nexus/src/app/ToastManager.h`
- Modify: `SCU_Nexus/src/app/ToastManager.cpp`
- Modify: `SCU_Nexus/main.cpp`
- Modify: `SCU_Nexus/qml/App.qml`
- Modify: `SCU_Nexus/qml/MainShell.qml`
- Modify: `SCU_Nexus/qml/LoginPage.qml`
- Modify: `SCU_Nexus/qml/SettingsPage.qml`
- Modify/Create: `SCU_Nexus/qml/components/*.qml`
- Create: `SCU_Nexus/qml/styles/Theme.qml`

**Interfaces:**
- Consumes: Person B `AuthViewModel` interface when present through `appController.authViewModel`.
- Produces: QML route shell and common components C/D can import.

- [ ] **Step 1: Add/adjust shell APIs**

Add `AppController.ready`, `loggedIn`, `appVersion`, `startupFinished`, `startupFailed`, `sessionExpired`, and placeholder `QObject* authViewModel()`.

- [ ] **Step 2: Strengthen router**

Add `replace(QString route)`, keep a back stack, expose `routeTitle`, and reject invalid routes without mutating state.

- [ ] **Step 3: Modernize UI**

Use a restrained campus-client design: fixed left navigation, compact header, consistent spacing, light/dark colors from `themeManager`, no nested cards, no business logic in QML.

- [ ] **Step 4: Add missing reusable components**

Add ToastHost, AppDialog, DataTable, FormRow, IconButton, ModuleHeader, SearchBar, SectionHeader. Keep properties from Person A docs stable.

- [ ] **Step 5: Build the app**

Run: `cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug --target SCU_Nexus`
Expected: PASS.

### Task 4: Person B Service Boundary Stubs

**Files:**
- Create: `SCU_Nexus/src/services/auth/AuthState.h`
- Create: `SCU_Nexus/src/services/auth/AuthErrors.h`
- Create: `SCU_Nexus/src/services/auth/AuthViewModel.h`
- Create: `SCU_Nexus/src/services/auth/AuthViewModel.cpp`
- Create: `SCU_Nexus/src/services/auth/ScuAuthService.h`
- Create: `SCU_Nexus/src/services/auth/ScuAuthService.cpp`
- Create: `SCU_Nexus/src/services/auth/ZhjwAuthService.h`
- Create: `SCU_Nexus/src/services/auth/ZhjwAuthService.cpp`
- Create: `SCU_Nexus/src/services/api/ZhjwApiService.h`
- Create: `SCU_Nexus/src/services/api/ZhjwApiService.cpp`
- Modify: `SCU_Nexus/CMakeLists.txt`

**Interfaces:**
- Produces: QML-visible AuthViewModel with `loggedIn`, `loading`, `captchaLoading`, `captchaImageUrl`, `errorMessage`, `fetchCaptcha()`, `login(...)`, `logout()`, and `clearError()`.
- Produces: C++ service classes with async-friendly signal/callback boundaries and no UI blocking.

- [ ] **Step 1: Add compileable service boundaries**

Create headers and minimal implementations that compile, expose documented interfaces, and keep real network behavior behind service classes.

- [ ] **Step 2: Wire AuthViewModel into AppController**

Expose AuthViewModel to QML and route LoginPage interactions through it.

- [ ] **Step 3: Build and run offline tests**

Run: `cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug --target SCU_Nexus person_b_foundation_tests && build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug/person_b_foundation_tests`
Expected: PASS.

## Self-Review

- Spec coverage: A shell, routing, theme, public components, login/settings shell, no QML business logic, and B cookie/parser/auth/API boundaries are covered.
- Deferred by design: real SM2 encryption and live campus login require additional dependency selection and real-account integration; this plan establishes the documented boundary and offline-testable behavior first.
- Placeholder scan: no `TBD` or undefined task references remain.
- Type consistency: route names and AuthViewModel property names match Person A/B documents.
