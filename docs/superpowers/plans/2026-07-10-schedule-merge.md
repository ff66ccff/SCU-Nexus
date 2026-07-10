# Schedule Branch Merge Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Merge `schedule` into `main` as a two-parent merge commit and integrate a document-compliant schedule module into the unified application.

**Architecture:** Preserve `main` as the application host and transplant only schedule-owned sources from the unrelated `schedule` root. Dependency injection in `main.cpp` connects local storage, the existing teaching-affairs API, and QML ViewModels. C++ tests cover domain and integration behavior; QML remains presentation and interaction only.

**Tech Stack:** C++20, Qt 6 Core/Gui/QML/Quick/QuickControls2/Network/Sql/Test, SQLite, CMake/CTest.

## Global Constraints

- The authoritative functional specification is `SCU_Nexus/docs/team/03_person_C_schedule_management.md`.
- Offline schedule viewing and editing must not depend on authentication or HTTP.
- Remote import must use `ZhjwApiService`; schedule code must not handle cookies, passwords, or URLs.
- The final repository contains one application rooted at `SCU_Nexus/` and no tracked build output.
- The final merge commit preserves both branch histories.

---

### Task 1: Establish the filtered merge

**Files:**
- Move: `src/models/*` to `SCU_Nexus/src/models/`
- Move: `src/repositories/*` to `SCU_Nexus/src/repositories/`
- Move: `src/services/course/*` to `SCU_Nexus/src/services/course/`
- Move: `src/viewmodels/*` to `SCU_Nexus/src/viewmodels/`
- Move: `qml/pages/schedule/*` to `SCU_Nexus/qml/pages/schedule/`
- Move: `qml/components/schedule/*` to `SCU_Nexus/qml/components/schedule/`
- Move: `tests/test_course.cpp` to `SCU_Nexus/tests/test_schedule.cpp`
- Modify: `.gitignore`

**Interfaces:**
- Consumes: the complete tree at `schedule` commit `ce9d9cc36da663c37a59660f60a4dfbb26ce77e3`
- Produces: schedule-owned files in the paths mandated by the person-C document

- [ ] **Step 1: Start the unrelated-history merge without committing**

Run: `git merge --allow-unrelated-histories --no-commit schedule`

Expected: merge stops for the add/add `.gitignore` conflict or leaves an uncommitted automatic merge.

- [ ] **Step 2: Retain the main ignore policy and remove generated output**

Resolve `.gitignore` from `main`; remove `.qtcreator/`, `build/`, `build_fix/`, and all standalone root project files introduced by `schedule`.

- [ ] **Step 3: Move schedule-owned sources into the unified application**

Use `git mv` for the exact file groups listed above so history remains visible across the merge.

- [ ] **Step 4: Verify the filtered tree**

Run: `git status --short` and `git ls-files | rg '^(build|build_fix|\\.qtcreator)/'`

Expected: schedule sources are under `SCU_Nexus/`; the generated-output search prints nothing.

### Task 2: Integrate C++ build and dependency injection

**Files:**
- Modify: `SCU_Nexus/CMakeLists.txt`
- Modify: `SCU_Nexus/main.cpp`
- Test: `SCU_Nexus/tests/test_schedule.cpp`

**Interfaces:**
- Consumes: `ZhjwApiService::{fetchSemesters,fetchJwxtSchedule,fetchCurrentWeek}` callback APIs
- Produces: QML context objects `scheduleViewModel`, `scheduleImportViewModel`, and `courseEditViewModel`

- [ ] **Step 1: Add a failing integration smoke test**

Add a QtTest case that initializes a temporary `ScheduleRepository`, injects it into the ViewModels, and verifies the empty-schedule state and creation path.

- [ ] **Step 2: Run the schedule test and observe the missing integration API**

Run: `cmake --build <build-dir> --target schedule_tests && ctest --test-dir <build-dir> -R schedule_tests --output-on-failure`

Expected: compilation or assertion failure identifies the missing integration behavior.

- [ ] **Step 3: Register sources, QML files, and schedule tests in CMake**

Add all schedule model/repository/service/ViewModel files to `SCU_Nexus`, all schedule page/component files to `qt_add_qml_module`, and a `schedule_tests` executable linked to `Qt6::Core`, `Qt6::Sql`, and `Qt6::Test`.

- [ ] **Step 4: Construct and expose the schedule objects**

In `main.cpp`, initialize `ScheduleRepository`, inject it into all schedule ViewModels, inject `ZhjwApiService` into the import ViewModel, connect refresh signals, and publish all three objects to QML.

- [ ] **Step 5: Re-run the focused test**

Run the schedule test command from Step 2.

Expected: the integration smoke test passes.

### Task 3: Complete document-required C++ behavior

**Files:**
- Modify: `SCU_Nexus/src/repositories/ScheduleRepository.{h,cpp}`
- Modify: `SCU_Nexus/src/viewmodels/ScheduleViewModel.{h,cpp}`
- Modify: `SCU_Nexus/src/viewmodels/ScheduleImportViewModel.{h,cpp}`
- Modify: `SCU_Nexus/src/viewmodels/CourseEditViewModel.{h,cpp}`
- Modify as failures require: schedule model and service files
- Test: `SCU_Nexus/tests/test_schedule.cpp`

**Interfaces:**
- Produces: schedule create/rename/delete/configuration methods, asynchronous semester loading/import/week sync, atomic import behavior, and conflict-safe course editing

- [ ] **Step 1: Add failing tests for uncovered acceptance behavior**

Cover update course, schedule rename/config save, cross-period validation, invalid repository operations, import rollback, layout track assignment, and API-independent import processing.

- [ ] **Step 2: Run tests and record expected failures**

Run the focused schedule CTest command.

Expected: each new test fails because the corresponding behavior is absent or incorrect.

- [ ] **Step 3: Implement the minimum document-compliant behavior**

Add the required repository transactions and ViewModel methods. Replace the import stub with callbacks to the injected `ZhjwApiService` while retaining `importFromJson` as the tested parsing boundary.

- [ ] **Step 4: Run the focused test until green**

Run the focused schedule CTest command.

Expected: all schedule tests pass with zero failures.

### Task 4: Integrate the schedule UI into the main shell

**Files:**
- Modify: `SCU_Nexus/qml/MainShell.qml`
- Rewrite as required: `SCU_Nexus/qml/pages/schedule/*.qml`
- Rewrite as required: `SCU_Nexus/qml/components/schedule/*.qml`

**Interfaces:**
- Consumes: the three injected schedule ViewModels and shared `Theme`, `Card`, `AppButton`, `AppDialog`, `EmptyView`, `ErrorView`, and `ToastHost`
- Produces: working empty state, week grid, course CRUD, schedule management/settings, import conflict handling, and current-week synchronization

- [ ] **Step 1: Route Schedule to the real page**

Replace `SchedulePagePlaceholder.qml` in `pageSource()` with `pages/schedule/SchedulePage.qml`.

- [ ] **Step 2: Bind the real ViewModels**

Replace commented handlers and local placeholder properties with ViewModel properties, invocations, and completion/error signals.

- [ ] **Step 3: Connect dialogs to document-required operations**

Wire course add/edit/delete, schedule create/rename/delete/switch, configuration save, semester fetch/import/conflict strategy, and current-week synchronization.

- [ ] **Step 4: Verify QML loading**

Run the application with the offscreen platform or the available QML lint target.

Expected: no missing type, property, signal, or context-object errors.

### Task 5: Full verification and merge commit

**Files:**
- Inspect all merged files and Git metadata

**Interfaces:**
- Produces: a verified two-parent commit on `main`

- [ ] **Step 1: Configure a clean build directory**

Run: `cmake -S SCU_Nexus -B <clean-build-dir>`

Expected: configuration exits 0.

- [ ] **Step 2: Build every target**

Run: `cmake --build <clean-build-dir> --parallel 2`

Expected: build exits 0.

- [ ] **Step 3: Run the complete test suite**

Run: `ctest --test-dir <clean-build-dir> --output-on-failure`

Expected: all registered tests pass.

- [ ] **Step 4: Audit requirements and repository hygiene**

Verify the person-C acceptance list against source/tests, search for schedule stubs and conflict markers, and confirm no generated files are tracked.

- [ ] **Step 5: Commit the merge**

Run: `git commit -m "Merge schedule branch into main"`

Expected: `git rev-list --parents -n 1 HEAD` prints the new commit plus both prior branch tips.

