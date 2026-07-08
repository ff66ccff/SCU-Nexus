cmake_minimum_required(VERSION 3.21)

project(SCU_Nexus VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Sql Test)

qt_standard_project_setup(REQUIRES 6.5)

set(SCU_NEXUS_SOURCES
    src/common/QueryState.cpp
    src/main.cpp
    src/models/AcademicCalendarModels.cpp
    src/models/ExamPlanModels.cpp
    src/models/GradeModels.cpp
    src/repositories/QueryCacheRepository.cpp
    src/services/calendar/AcademicCalendarService.cpp
    src/services/calendar/HolidayService.cpp
    src/services/grades/GradeStatisticsService.cpp
    src/services/zhjw/MockZhjwApiService.cpp
    src/viewmodels/AcademicCalendarViewModel.cpp
    src/viewmodels/ExamPlanViewModel.cpp
    src/viewmodels/GradesViewModel.cpp
    src/viewmodels/QueryCacheViewModel.cpp
    src/widgets/AcademicCalendarWidget.cpp
    src/widgets/AppStyle.cpp
    src/widgets/CampusBackdrop.cpp
    src/widgets/ExamPlanWidget.cpp
    src/widgets/GradesWidget.cpp
    src/widgets/MainWindow.cpp
    src/widgets/QueryStateWidget.cpp
)

qt_add_executable(SCU_Nexus ${SCU_NEXUS_SOURCES})

target_include_directories(SCU_Nexus PRIVATE src)

target_link_libraries(SCU_Nexus
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        Qt6::Sql
)

if(WIN32)
    get_filename_component(QT_BIN_DIR "${Qt6_DIR}/../../../bin" ABSOLUTE)
    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt
        HINTS "${QT_BIN_DIR}"
        NO_DEFAULT_PATH
    )

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET SCU_Nexus POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                    --no-translations
                    "$<TARGET_FILE:SCU_Nexus>"
            COMMENT "Deploying Qt runtime files with windeployqt"
            VERBATIM
        )
    else()
        message(WARNING "windeployqt was not found. Double-clicking SCU_Nexus.exe may fail unless Qt bin and plugins are on PATH.")
    endif()
endif()

enable_testing()

qt_add_executable(SCU_Nexus_Tests
    tests/tst_d_module.cpp
    src/models/AcademicCalendarModels.cpp
    src/models/ExamPlanModels.cpp
    src/models/GradeModels.cpp
    src/repositories/QueryCacheRepository.cpp
    src/services/calendar/AcademicCalendarService.cpp
    src/services/calendar/HolidayService.cpp
    src/services/grades/GradeStatisticsService.cpp
)

target_include_directories(SCU_Nexus_Tests PRIVATE src)
target_link_libraries(SCU_Nexus_Tests PRIVATE Qt6::Core Qt6::Network Qt6::Sql Qt6::Test)
add_test(NAME SCU_Nexus_Tests COMMAND SCU_Nexus_Tests)

if(WIN32 AND WINDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET SCU_Nexus_Tests POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                --no-translations
                "$<TARGET_FILE:SCU_Nexus_Tests>"
        COMMENT "Deploying Qt test runtime files with windeployqt"
        VERBATIM
    )
endif()
