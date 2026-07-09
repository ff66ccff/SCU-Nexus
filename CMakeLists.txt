cmake_minimum_required(VERSION 3.16)
project(SCU_Nexus VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    Sql
    Widgets
)

# If Qt6 not found, try Qt5
if(NOT Qt6_FOUND)
    find_package(Qt5 5.15 REQUIRED COMPONENTS
        Core
        Gui
        Qml
        Quick
        QuickControls2
        Sql
        Widgets
    )
endif()

# Source files
set(MODEL_SOURCES
    src/models/TimeSlot.cpp
    src/models/ScheduleConfig.cpp
    src/models/Course.cpp
)

set(MODEL_HEADERS
    src/models/WeekType.h
    src/models/TimeSlot.h
    src/models/ScheduleConfig.h
    src/models/Course.h
)

set(REPOSITORY_SOURCES
    src/repositories/ScheduleRepository.cpp
)

set(REPOSITORY_HEADERS
    src/repositories/ScheduleRepository.h
)

set(SERVICE_SOURCES
    src/services/course/JwxtScheduleParser.cpp
    src/services/course/ScheduleImportService.cpp
    src/services/course/CourseLayoutService.cpp
)

set(SERVICE_HEADERS
    src/services/course/JwxtScheduleParser.h
    src/services/course/ScheduleImportService.h
    src/services/course/CourseLayoutService.h
)

set(VIEWMODEL_SOURCES
    src/viewmodels/ScheduleViewModel.cpp
    src/viewmodels/ScheduleImportViewModel.cpp
    src/viewmodels/CourseEditViewModel.cpp
)

set(VIEWMODEL_HEADERS
    src/viewmodels/ScheduleViewModel.h
    src/viewmodels/ScheduleImportViewModel.h
    src/viewmodels/CourseEditViewModel.h
)

set(QML_RESOURCES
    qml.qrc
)

# Create executable
add_executable(${PROJECT_NAME}
    main.cpp
    ${MODEL_SOURCES}
    ${MODEL_HEADERS}
    ${REPOSITORY_SOURCES}
    ${REPOSITORY_HEADERS}
    ${SERVICE_SOURCES}
    ${SERVICE_HEADERS}
    ${VIEWMODEL_SOURCES}
    ${VIEWMODEL_HEADERS}
    ${QML_RESOURCES}
)

# Link Qt libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
    Qt::QuickControls2
    Qt::Sql
    Qt::Widgets
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Platform-specific settings
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Install
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Tests (optional)
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()
