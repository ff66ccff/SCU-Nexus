cmake_minimum_required(VERSION 3.30)

project(SCU_Nexus VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    Network
    Sql
    Test
)

if(COMMAND qt_policy)
    qt_policy(SET QTP0004 NEW)
endif()

qt_standard_project_setup()

set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(OPENSSL_ROOT_DIR "D:/QT/Tools/mingw1310_64/opt")
set(OPENSSL_CRYPTO_LIBRARY "${OPENSSL_ROOT_DIR}/lib/libcrypto.dll.a")
set(OPENSSL_CRYPTO_RUNTIME "${OPENSSL_ROOT_DIR}/bin/libcrypto-1_1-x64.dll")

qt_add_executable(SCU_Nexus
    main.cpp
    src/app/AppController.h
    src/app/AppController.cpp
    src/app/Router.h
    src/app/Router.cpp
    src/app/AppSettings.h
    src/app/AppSettings.cpp
    src/app/ThemeManager.h
    src/app/ThemeManager.cpp
    src/app/ToastManager.h
    src/app/ToastManager.cpp
    src/core/crypto/Sm2Crypto.h
    src/core/crypto/Sm2Crypto.cpp
    src/core/logging/AuthLogger.h
    src/core/logging/AuthLogger.cpp
    src/core/network/CookieJar.h
    src/core/network/CookieJar.cpp
    src/core/network/CookieHttpClient.h
    src/core/network/CookieHttpClient.cpp
    src/core/network/HttpRequest.h
    src/core/network/HttpResponse.h
    src/core/network/NetworkError.h
    src/core/network/NetworkSettings.h
    src/services/auth/AuthState.h
    src/services/auth/AuthErrors.h
    src/services/auth/AuthViewModel.h
    src/services/auth/AuthViewModel.cpp
    src/services/auth/ScuAuthService.h
    src/services/auth/ScuAuthService.cpp
    src/services/auth/ZhjwAuthService.h
    src/services/auth/ZhjwAuthService.cpp
    src/services/api/ApiDtos.h
    src/services/api/ZhjwParsers.h
    src/services/api/ZhjwParsers.cpp
    src/services/api/ZhjwApiService.h
    src/services/api/ZhjwApiService.cpp
    src/common/QueryState.h
    src/common/QueryState.cpp
    src/models/AcademicCalendarModels.h
    src/models/AcademicCalendarModels.cpp
    src/models/ExamPlanModels.h
    src/models/ExamPlanModels.cpp
    src/models/GradeModels.h
    src/models/GradeModels.cpp
    src/repositories/QueryCacheRepository.h
    src/repositories/QueryCacheRepository.cpp
    src/services/calendar/AcademicCalendarService.h
    src/services/calendar/AcademicCalendarService.cpp
    src/services/calendar/HolidayService.h
    src/services/calendar/HolidayService.cpp
    src/services/grades/GradeStatisticsService.h
    src/services/grades/GradeStatisticsService.cpp
    src/services/zhjw/ZhjwQueryService.h
    src/services/zhjw/ZhjwQueryService.cpp
    src/services/zhjw/ZhjwApiQueryService.cpp
    src/viewmodels/AcademicCalendarViewModel.h
    src/viewmodels/AcademicCalendarViewModel.cpp
    src/viewmodels/ExamPlanViewModel.h
    src/viewmodels/ExamPlanViewModel.cpp
    src/viewmodels/GradesViewModel.h
    src/viewmodels/GradesViewModel.cpp
    src/viewmodels/QueryCacheViewModel.h
    src/viewmodels/QueryCacheViewModel.cpp
    src/models/Course.h
    src/models/Course.cpp
    src/models/ScheduleConfig.h
    src/models/ScheduleConfig.cpp
    src/models/TimeSlot.h
    src/models/TimeSlot.cpp
    src/models/WeekType.h
    src/repositories/ScheduleRepository.h
    src/repositories/ScheduleRepository.cpp
    src/services/course/CourseLayoutService.h
    src/services/course/CourseLayoutService.cpp
    src/services/course/JwxtScheduleParser.h
    src/services/course/JwxtScheduleParser.cpp
    src/services/course/ScheduleImportService.h
    src/services/course/ScheduleImportService.cpp
    src/viewmodels/ScheduleViewModel.h
    src/viewmodels/ScheduleViewModel.cpp
    src/viewmodels/ScheduleImportViewModel.h
    src/viewmodels/ScheduleImportViewModel.cpp
    src/viewmodels/CourseEditViewModel.h
    src/viewmodels/CourseEditViewModel.cpp
)

set_source_files_properties(qml/styles/Theme.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(SCU_Nexus
    URI SCU_Nexus
    VERSION 1.0
    RESOURCE_PREFIX /
    QML_FILES
        qml/App.qml
        qml/MainShell.qml
        qml/LoginPage.qml
        qml/SettingsPage.qml
        qml/styles/Theme.qml
        qml/components/AppButton.qml
        qml/components/AppDialog.qml
        qml/components/AppTextField.qml
        qml/components/Card.qml
        qml/components/DataTable.qml
        qml/components/EmptyView.qml
        qml/components/ErrorView.qml
        qml/components/FormRow.qml
        qml/components/IconButton.qml
        qml/components/LoadingView.qml
        qml/components/LoginRequiredView.qml
        qml/components/ModuleHeader.qml
        qml/components/SearchBar.qml
        qml/components/SectionHeader.qml
        qml/components/SegmentedControl.qml
        qml/components/ToastHost.qml
        qml/components/query/LastUpdatedLabel.qml
        qml/components/query/RefreshButton.qml
        qml/components/query/QueryStatePane.qml
        qml/pages/AcademicCalendarPagePlaceholder.qml
        qml/pages/ExamPlanPagePlaceholder.qml
        qml/pages/GradesPagePlaceholder.qml
        qml/pages/calendar/AcademicCalendarPage.qml
        qml/pages/calendar/CalendarImageViewer.qml
        qml/pages/exam/ExamPlanPage.qml
        qml/pages/exam/ExamCard.qml
        qml/pages/grades/GradesPage.qml
        qml/pages/grades/SchemeScoresTab.qml
        qml/pages/grades/PassingScoresTab.qml
        qml/pages/grades/CustomStatsTab.qml
        qml/pages/grades/GradeCourseCard.qml
        qml/pages/grades/GradeSummaryPanel.qml
        qml/pages/schedule/SchedulePage.qml
        qml/pages/schedule/CourseEditDialog.qml
        qml/pages/schedule/ImportScheduleDialog.qml
        qml/pages/schedule/ScheduleManagementDialog.qml
        qml/pages/schedule/ScheduleSettingsDialog.qml
        qml/pages/schedule/TimeSlotSettingsDialog.qml
        qml/components/schedule/CourseGrid.qml
        qml/components/schedule/CourseCard.qml
        qml/components/schedule/WeekSwitcher.qml
        qml/components/schedule/DayColumn.qml
        qml/components/schedule/SectionColumn.qml
        qml/components/schedule/CourseConflictBadge.qml
)

target_link_libraries(SCU_Nexus PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    Qt6::Sql
    ${OPENSSL_CRYPTO_LIBRARY}
)

target_include_directories(SCU_Nexus PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OPENSSL_ROOT_DIR}/include
)

qt_add_executable(person_b_foundation_tests
    tests/test_person_b_foundation.cpp
    src/app/AppController.h
    src/app/AppController.cpp
    src/core/crypto/Sm2Crypto.h
    src/core/crypto/Sm2Crypto.cpp
    src/core/logging/AuthLogger.h
    src/core/logging/AuthLogger.cpp
    src/core/network/CookieJar.h
    src/core/network/CookieJar.cpp
    src/core/network/CookieHttpClient.h
    src/core/network/CookieHttpClient.cpp
    src/core/network/HttpRequest.h
    src/core/network/HttpResponse.h
    src/core/network/NetworkError.h
    src/core/network/NetworkSettings.h
    src/services/auth/AuthViewModel.h
    src/services/auth/AuthViewModel.cpp
    src/services/auth/ScuAuthService.h
    src/services/auth/ScuAuthService.cpp
    src/services/auth/ZhjwAuthService.h
    src/services/auth/ZhjwAuthService.cpp
    src/services/api/ApiDtos.h
    src/services/api/ZhjwParsers.h
    src/services/api/ZhjwParsers.cpp
    src/services/api/ZhjwApiService.h
    src/services/api/ZhjwApiService.cpp
    src/services/zhjw/ZhjwQueryService.h
    src/services/zhjw/ZhjwQueryService.cpp
    src/services/zhjw/ZhjwApiQueryService.cpp
)

target_include_directories(person_b_foundation_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OPENSSL_ROOT_DIR}/include
)

target_link_libraries(person_b_foundation_tests PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Test
    ${OPENSSL_CRYPTO_LIBRARY}
)

enable_testing()
add_test(NAME person_b_foundation_tests COMMAND person_b_foundation_tests)

qt_add_executable(person_d_query_tests
    tests/test_person_d_queries.cpp
    src/common/QueryState.h
    src/common/QueryState.cpp
    src/models/AcademicCalendarModels.h
    src/models/AcademicCalendarModels.cpp
    src/models/ExamPlanModels.h
    src/models/ExamPlanModels.cpp
    src/models/GradeModels.h
    src/models/GradeModels.cpp
    src/repositories/QueryCacheRepository.h
    src/repositories/QueryCacheRepository.cpp
    src/services/calendar/AcademicCalendarService.h
    src/services/calendar/AcademicCalendarService.cpp
    src/services/calendar/HolidayService.h
    src/services/calendar/HolidayService.cpp
    src/services/grades/GradeStatisticsService.h
    src/services/grades/GradeStatisticsService.cpp
    src/services/zhjw/ZhjwQueryService.h
    src/services/zhjw/ZhjwQueryService.cpp
    src/viewmodels/AcademicCalendarViewModel.h
    src/viewmodels/AcademicCalendarViewModel.cpp
    src/viewmodels/ExamPlanViewModel.h
    src/viewmodels/ExamPlanViewModel.cpp
    src/viewmodels/GradesViewModel.h
    src/viewmodels/GradesViewModel.cpp
    src/viewmodels/QueryCacheViewModel.h
    src/viewmodels/QueryCacheViewModel.cpp
)

target_include_directories(person_d_query_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OPENSSL_ROOT_DIR}/include
)

target_link_libraries(person_d_query_tests PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Sql
    Qt6::Test
    ${OPENSSL_CRYPTO_LIBRARY}
)

add_test(NAME person_d_query_tests COMMAND person_d_query_tests)

qt_add_executable(schedule_tests
    tests/test_schedule.cpp
    src/models/TimeSlot.h
    src/models/TimeSlot.cpp
    src/models/ScheduleConfig.h
    src/models/ScheduleConfig.cpp
    src/models/Course.h
    src/models/Course.cpp
    src/models/WeekType.h
    src/repositories/ScheduleRepository.h
    src/repositories/ScheduleRepository.cpp
    src/services/course/JwxtScheduleParser.h
    src/services/course/JwxtScheduleParser.cpp
    src/services/course/ScheduleImportService.h
    src/services/course/ScheduleImportService.cpp
    src/services/course/CourseLayoutService.h
    src/services/course/CourseLayoutService.cpp
    src/viewmodels/ScheduleViewModel.h
    src/viewmodels/ScheduleViewModel.cpp
    src/viewmodels/CourseEditViewModel.h
    src/viewmodels/CourseEditViewModel.cpp
    src/viewmodels/ScheduleImportViewModel.h
    src/viewmodels/ScheduleImportViewModel.cpp
)

target_include_directories(schedule_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(schedule_tests PRIVATE
    Qt6::Core
    Qt6::Sql
    Qt6::Test
)

add_test(NAME schedule_tests COMMAND schedule_tests)

qt_add_executable(app_foundation_tests
    tests/test_app_foundation.cpp
    src/app/AppController.h
    src/app/AppController.cpp
    src/app/AppSettings.h
    src/app/AppSettings.cpp
    src/common/QueryState.h
    src/common/QueryState.cpp
    src/core/crypto/Sm2Crypto.h
    src/core/crypto/Sm2Crypto.cpp
    src/core/logging/AuthLogger.h
    src/core/logging/AuthLogger.cpp
    src/core/network/CookieJar.h
    src/core/network/CookieJar.cpp
    src/core/network/CookieHttpClient.h
    src/core/network/CookieHttpClient.cpp
    src/core/network/HttpRequest.h
    src/core/network/HttpResponse.h
    src/core/network/NetworkError.h
    src/core/network/NetworkSettings.h
    src/services/auth/AuthErrors.h
    src/services/auth/AuthViewModel.h
    src/services/auth/AuthViewModel.cpp
    src/services/auth/ScuAuthService.h
    src/services/auth/ScuAuthService.cpp
    src/repositories/QueryCacheRepository.h
    src/repositories/QueryCacheRepository.cpp
    src/viewmodels/QueryCacheViewModel.h
    src/viewmodels/QueryCacheViewModel.cpp
)

target_include_directories(app_foundation_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OPENSSL_ROOT_DIR}/include
)

target_link_libraries(app_foundation_tests PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Sql
    Qt6::Test
    ${OPENSSL_CRYPTO_LIBRARY}
)

add_test(NAME app_foundation_tests COMMAND app_foundation_tests)
set_tests_properties(app_foundation_tests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

set_target_properties(SCU_Nexus PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

add_custom_command(TARGET SCU_Nexus POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OPENSSL_CRYPTO_RUNTIME}"
        "$<TARGET_FILE_DIR:SCU_Nexus>"
)
