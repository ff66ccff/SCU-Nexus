# Test CMakeLists.txt for SCU_Nexus tests

cmake_minimum_required(VERSION 3.16)

find_package(Qt6 REQUIRED COMPONENTS Test Sql)

set(TEST_SOURCES
    test_course.cpp
    ../src/models/TimeSlot.cpp
    ../src/models/ScheduleConfig.cpp
    ../src/models/Course.cpp
    ../src/repositories/ScheduleRepository.cpp
    ../src/services/course/JwxtScheduleParser.cpp
    ../src/services/course/ScheduleImportService.cpp
    ../src/services/course/CourseLayoutService.cpp
)

add_executable(test_course ${TEST_SOURCES})

target_include_directories(test_course PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(test_course PRIVATE
    Qt::Test
    Qt::Sql
)

add_test(NAME test_course COMMAND test_course)
