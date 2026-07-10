# Schedule Branch Merge Design

## Goal

Merge the unrelated `schedule` branch into `main` while preserving both histories and delivering the schedule module inside the existing `SCU_Nexus/` application described by `SCU_Nexus/docs/team/03_person_C_schedule_management.md`.

## Decision

Create a real merge commit with `main` and `schedule` as parents. Treat `main` as authoritative for repository layout, application shell, authentication, remote API access, shared QML components, and build configuration. Treat `schedule` as the source for schedule-domain models, SQLite storage, layout logic, schedule ViewModels, schedule QML, and schedule tests.

The root-level standalone application, Qt Creator metadata, and compiled output from `schedule` are discarded. Schedule-owned sources are moved under `SCU_Nexus/src`, `SCU_Nexus/qml`, and `SCU_Nexus/tests`.

## Integration Boundaries

- `ScheduleRepository` owns local SQLite schedules and courses and remains usable offline.
- `ScheduleViewModel` owns week navigation, course display, schedule management, and configuration updates.
- `ScheduleImportViewModel` calls the existing `ZhjwApiService` callback API for semesters, schedule JSON, and current week; it never performs HTTP or authentication itself.
- `MainShell.qml` routes the Schedule item to the real schedule page.
- `main.cpp` constructs one repository and the three schedule ViewModels, injects dependencies, and exposes them to QML.
- The merged QML uses the main branch's application shell and theme; it does not introduce a second `ApplicationWindow`.

## Correctness Rules

The person-C development document is authoritative when implementations disagree. Existing schedule code may be retained only when its behavior matches that document. Missing or stubbed behavior is implemented with a failing regression test first where it can be exercised in C++.

The merge excludes `build/`, `build_fix/`, `.qtcreator/`, standalone `CMakeLists.txt`, `SCU_Nexus.pro`, root `main.cpp`, root `qml.qrc`, and root `qml/main.qml` from the final tree.

## Verification

- Confirm the final commit has both `main` and `schedule` as parents.
- Configure and build the unified CMake project.
- Run the full CTest suite, including schedule model, repository, parser, import, layout, and ViewModel tests.
- Run static QML checks available in the Qt installation or load the application offscreen long enough to catch QML component errors.
- Verify no build output or second application tree is tracked.

