cmake_minimum_required(VERSION 3.24)

project(SCU_Nexus VERSION 0.1.0 LANGUAGES CXX)

include(CTest)
include(GNUInstallDirs)

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
)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
endif()
if(MINGW)
    find_path(Iconv_INCLUDE_DIR
        NAMES iconv.h
        HINTS ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES}
    )
    find_library(Iconv_LIBRARY
        NAMES iconv libiconv
        HINTS ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES}
    )
endif()
find_package(Iconv REQUIRED)

# Qt's MinGW kit ships its matching OpenSSL SDK in <compiler>/opt, but
# Qt Creator does not add that directory to CMake's search path by default.
if(WIN32 AND MINGW AND NOT OPENSSL_ROOT_DIR AND CMAKE_CXX_COMPILER)
    get_filename_component(_scu_nexus_compiler_bin
        "${CMAKE_CXX_COMPILER}" DIRECTORY
    )
    get_filename_component(_scu_nexus_compiler_root
        "${_scu_nexus_compiler_bin}" DIRECTORY
    )
    set(_scu_nexus_qt_openssl_root "${_scu_nexus_compiler_root}/opt")
    if(EXISTS "${_scu_nexus_qt_openssl_root}/include/openssl/opensslv.h")
        set(OPENSSL_ROOT_DIR "${_scu_nexus_qt_openssl_root}"
            CACHE PATH "OpenSSL root auto-detected from the Qt MinGW kit"
        )
    endif()
endif()

find_package(OpenSSL 1.1 REQUIRED COMPONENTS Crypto)

if(COMMAND qt_policy)
    qt_policy(SET QTP0004 NEW)
endif()

qt_standard_project_setup()

set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(SCU_NEXUS_OPENSSL_RUNTIME "" CACHE FILEPATH
    "Path to the OpenSSL libcrypto DLL copied into Windows packages")

if(WIN32 AND NOT SCU_NEXUS_OPENSSL_RUNTIME AND OPENSSL_ROOT_DIR)
    find_file(SCU_NEXUS_OPENSSL_RUNTIME
        NAMES libcrypto-3-x64.dll libcrypto-1_1-x64.dll libcrypto-1_1.dll
        PATHS "${OPENSSL_ROOT_DIR}/bin"
        NO_DEFAULT_PATH
    )
endif()

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
    src/models/ClassroomModels.h
    src/models/ClassroomModels.cpp
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
    src/viewmodels/ClassroomViewModel.h
    src/viewmodels/ClassroomViewModel.cpp
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
    src/services/course/CourseValidator.h
    src/services/course/CourseValidator.cpp
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

target_compile_definitions(SCU_Nexus PRIVATE
    SCU_NEXUS_VERSION="${PROJECT_VERSION}"
)

if(WIN32)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/windows.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/windows.rc"
        @ONLY
    )
    target_sources(SCU_Nexus PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/windows.rc")
endif()

set_source_files_properties(qml/styles/Theme.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(SCU_Nexus
    URI SCU_Nexus
    VERSION 1.0
    RESOURCE_PREFIX /
    QML_FILES
        qml/App.qml
        qml/AppNavigation.qml
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
        qml/components/SourceAttribution.qml
        qml/components/ToastHost.qml
        qml/components/query/LastUpdatedLabel.qml
        qml/components/query/RefreshButton.qml
        qml/components/query/QueryStatePane.qml
        qml/pages/calendar/AcademicCalendarPage.qml
        qml/pages/calendar/CalendarImageViewer.qml
        qml/pages/classroom/ClassroomPage.qml
        qml/pages/classroom/ClassroomRoomCard.qml
        qml/pages/classroom/ClassroomDetailDialog.qml
        qml/pages/classroom/ClassroomDateDialog.qml
        qml/pages/classroom/ClassroomPeriodDialog.qml
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
    Iconv::Iconv
    OpenSSL::Crypto
)

target_include_directories(SCU_Nexus PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(BUILD_TESTING)
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
)

target_link_libraries(person_b_foundation_tests PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Test
    OpenSSL::Crypto
)

add_test(NAME person_b_foundation_tests COMMAND person_b_foundation_tests)

qt_add_executable(person_d_query_tests
    tests/test_person_d_queries.cpp
    src/core/logging/AuthLogger.h
    src/core/logging/AuthLogger.cpp
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
)

target_link_libraries(person_d_query_tests PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Sql
    Qt6::Test
    Iconv::Iconv
    OpenSSL::Crypto
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
    src/services/course/CourseValidator.h
    src/services/course/CourseValidator.cpp
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
)

target_link_libraries(app_foundation_tests PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Sql
    Qt6::Test
    OpenSSL::Crypto
)

add_test(NAME app_foundation_tests COMMAND app_foundation_tests)
set_tests_properties(app_foundation_tests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_add_executable(ui_contract_tests
    tests/test_ui_contracts.cpp
)

target_compile_definitions(ui_contract_tests PRIVATE
    SCU_NEXUS_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(ui_contract_tests PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME ui_contract_tests COMMAND ui_contract_tests)

qt_add_executable(classroom_query_tests
    tests/test_classroom_query.cpp
    src/common/QueryState.h
    src/common/QueryState.cpp
    src/core/network/NetworkError.h
    src/services/api/ApiDtos.h
    src/services/zhjw/ZhjwQueryService.h
    src/services/zhjw/ZhjwQueryService.cpp
    src/models/ClassroomModels.h
    src/models/ClassroomModels.cpp
    src/models/TimeSlot.h
    src/models/TimeSlot.cpp
    src/models/ScheduleConfig.h
    src/models/ScheduleConfig.cpp
    src/viewmodels/ClassroomViewModel.h
    src/viewmodels/ClassroomViewModel.cpp
)

target_include_directories(classroom_query_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(classroom_query_tests PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME classroom_query_tests COMMAND classroom_query_tests)

qt_add_executable(grades_qml_tests
    tests/test_grades_qml.cpp
    src/app/Router.h
    src/app/Router.cpp
    src/common/QueryState.h
    src/common/QueryState.cpp
    src/core/network/NetworkError.h
    src/models/GradeModels.h
    src/models/GradeModels.cpp
    src/repositories/QueryCacheRepository.h
    src/repositories/QueryCacheRepository.cpp
    src/services/api/ApiDtos.h
    src/services/grades/GradeStatisticsService.h
    src/services/grades/GradeStatisticsService.cpp
    src/services/zhjw/ZhjwQueryService.h
    src/services/zhjw/ZhjwQueryService.cpp
    src/viewmodels/GradesViewModel.h
    src/viewmodels/GradesViewModel.cpp
)

target_compile_definitions(grades_qml_tests PRIVATE
    SCU_NEXUS_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(grades_qml_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(grades_qml_tests PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Sql
    Qt6::Test
)

add_test(NAME grades_qml_tests COMMAND grades_qml_tests)
set_tests_properties(grades_qml_tests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QPA_FONTDIR=C:/Windows/Fonts"
)
endif()

set_target_properties(SCU_Nexus PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

if(WIN32 AND SCU_NEXUS_OPENSSL_RUNTIME)
    add_custom_command(TARGET SCU_Nexus POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SCU_NEXUS_OPENSSL_RUNTIME}"
            "$<TARGET_FILE_DIR:SCU_Nexus>"
        VERBATIM
    )
endif()

install(TARGETS SCU_Nexus
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET SCU_Nexus
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT "${deploy_script}")

if(WIN32 AND SCU_NEXUS_OPENSSL_RUNTIME)
    install(FILES "${SCU_NEXUS_OPENSSL_RUNTIME}"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/../README.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE"
    DESTINATION .
)

set(CPACK_PACKAGE_NAME "SCU-Nexus")
set(CPACK_PACKAGE_VENDOR "SCU-Nexus Contributors")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "四川大学学生的一站式校园信息助手")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/ff66ccff/SCU-Nexus")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "SCU-Nexus")
set(CPACK_PACKAGE_EXECUTABLES "SCU_Nexus" "SCU-Nexus")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/../README.md")
set(CPACK_PACKAGE_CHECKSUM SHA256)
set(CPACK_GENERATOR "ZIP")

if(WIN32)
    set(CPACK_PACKAGE_FILE_NAME "SCU-Nexus-${PROJECT_VERSION}-windows-x86_64")
    set(CPACK_NSIS_DISPLAY_NAME "SCU-Nexus")
    set(CPACK_NSIS_PACKAGE_NAME "SCU-Nexus")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN "SCU_Nexus.exe")
endif()

include(CPack)
