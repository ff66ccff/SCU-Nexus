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
        qml/components/ToastHost.qml
        qml/pages/SchedulePagePlaceholder.qml
        qml/pages/AcademicCalendarPagePlaceholder.qml
        qml/pages/ExamPlanPagePlaceholder.qml
        qml/pages/GradesPagePlaceholder.qml
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
    ${OPENSSL_ROOT_DIR}/include
)

qt_add_executable(person_b_foundation_tests
    tests/test_person_b_foundation.cpp
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

set_target_properties(SCU_Nexus PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

add_custom_command(TARGET SCU_Nexus POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OPENSSL_CRYPTO_RUNTIME}"
        "$<TARGET_FILE_DIR:SCU_Nexus>"
)
