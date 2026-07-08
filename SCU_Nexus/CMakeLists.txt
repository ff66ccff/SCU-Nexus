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
)

qt_standard_project_setup()

set(CMAKE_AUTORCC ON)

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
    qml.qrc
)

target_link_libraries(SCU_Nexus PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    Qt6::Sql
)

set_target_properties(SCU_Nexus PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)
