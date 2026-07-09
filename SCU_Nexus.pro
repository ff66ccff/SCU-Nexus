# SCU_Nexus - Qt Project File
# Supports both qmake and CMake builds

QT += core gui qml quick quickcontrols2 sql widgets

CONFIG += c++17

TARGET = SCU_Nexus
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated
DEFINES += QT_DEPRECATED_WARNINGS

# Source files
SOURCES += \
    main.cpp \
    src/models/TimeSlot.cpp \
    src/models/ScheduleConfig.cpp \
    src/models/Course.cpp \
    src/repositories/ScheduleRepository.cpp \
    src/services/course/JwxtScheduleParser.cpp \
    src/services/course/ScheduleImportService.cpp \
    src/services/course/CourseLayoutService.cpp \
    src/viewmodels/ScheduleViewModel.cpp \
    src/viewmodels/ScheduleImportViewModel.cpp \
    src/viewmodels/CourseEditViewModel.cpp

HEADERS += \
    src/models/WeekType.h \
    src/models/TimeSlot.h \
    src/models/ScheduleConfig.h \
    src/models/Course.h \
    src/repositories/ScheduleRepository.h \
    src/services/course/JwxtScheduleParser.h \
    src/services/course/ScheduleImportService.h \
    src/services/course/CourseLayoutService.h \
    src/viewmodels/ScheduleViewModel.h \
    src/viewmodels/ScheduleImportViewModel.h \
    src/viewmodels/CourseEditViewModel.h

RESOURCES += \
    qml.qrc

INCLUDEPATH += \
    $$PWD/src

# Platform-specific
win32 {
    RC_ICONS = app.ico
}

macx {
    ICON = app.icns
    QMAKE_INFO_PLIST = Info.plist
}
