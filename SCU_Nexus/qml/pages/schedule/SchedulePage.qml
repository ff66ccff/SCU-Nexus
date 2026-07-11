pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"
import "../../components/schedule"
import "../../styles"

Page {
    id: root

    background: Rectangle { color: Theme.background }

    function colorString(argb) {
        var rgb = Number(argb) % 16777216
        return "#" + ("000000" + rgb.toString(16)).slice(-6).toUpperCase()
    }

    function argbValue(colorText) {
        return 4278190080 + parseInt(colorText.slice(1), 16)
    }

    function populateCourseDialog() {
        courseDialog.isEditMode = courseEditViewModel.isEditMode
        courseDialog.courseId = courseEditViewModel.courseId
        courseDialog.courseName = courseEditViewModel.name
        courseDialog.courseTeacher = courseEditViewModel.teacher
        courseDialog.courseLocation = courseEditViewModel.location
        courseDialog.startWeek = courseEditViewModel.startWeek
        courseDialog.endWeek = courseEditViewModel.endWeek
        courseDialog.dayOfWeek = courseEditViewModel.dayOfWeek
        courseDialog.startSection = courseEditViewModel.startSection
        courseDialog.endSection = courseEditViewModel.endSection
        courseDialog.weekType = courseEditViewModel.weekType
        courseDialog.courseColor = colorString(courseEditViewModel.colorValue)
        courseDialog.errorText = ""
    }

    function openCourseForAdd(day, section) {
        courseEditViewModel.initForAdd(day || 1, section || 1)
        populateCourseDialog()
        courseDialog.open()
    }

    function openCourseForEdit(courseId) {
        courseEditViewModel.initForEdit(courseId)
        if (courseEditViewModel.errorMessage.length > 0) {
            toastManager.show(courseEditViewModel.errorMessage, "error")
            return
        }
        populateCourseDialog()
        courseDialog.open()
    }

    function openSettings() {
        var config = scheduleViewModel.currentConfig()
        settingsDialog.semesterName = config.semesterName || ""
        settingsDialog.semesterStartDate = config.semesterStartDate || ""
        settingsDialog.totalWeeks = config.totalWeeks || 20
        settingsDialog.morningSections = config.morningSections || 4
        settingsDialog.afternoonSections = config.afternoonSections || 5
        settingsDialog.eveningSections = config.eveningSections === undefined ? 3 : config.eveningSections
        settingsDialog.courseDuration = config.courseDuration || 45
        settingsDialog.breakDuration = config.breakDuration || 10
        settingsDialog.autoSyncTime = config.autoSyncTime === undefined ? true : config.autoSyncTime
        settingsDialog.timeSlots = config.timeSlots || []
        settingsDialog.timeSlotPreset = config.timeSlotPreset || "custom"
        settingsDialog.errorText = ""
        settingsDialog.open()
    }

    Component.onCompleted: scheduleViewModel.load()

    Connections {
        target: scheduleImportViewModel
        function onConflictChanged() {
            if (scheduleImportViewModel.hasConflict)
                conflictDialog.open()
            else
                conflictDialog.close()
        }
        function onImportFinished(scheduleId) {
            importDialog.close()
            scheduleViewModel.load()
            toastManager.show(scheduleImportViewModel.statusMessage, "success")
        }
        function onErrorChanged() {
            if (scheduleImportViewModel.errorMessage.length > 0)
                toastManager.show(scheduleImportViewModel.errorMessage, "error")
        }
        function onCurrentWeekSynced(week) {
            scheduleViewModel.load()
            scheduleViewModel.updateCurrentWeek(week)
        }
        function onStatusChanged() {
            if (!scheduleImportViewModel.importComplete
                    && scheduleImportViewModel.statusMessage.length > 0) {
                toastManager.show(scheduleImportViewModel.statusMessage, "success")
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pagePadding
        spacing: Theme.sectionGap

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ModuleHeader {
                Layout.fillWidth: true
                title: scheduleViewModel.hasSchedule
                       ? scheduleViewModel.currentScheduleName : "课表"
                subtitle: scheduleViewModel.hasSchedule
                          ? "第 " + scheduleViewModel.currentWeek + " 周，共 "
                            + scheduleViewModel.totalWeeks + " 周"
                          : "离线查看、编辑和管理本地课表"
            }

            AppButton {
                text: "导入课表"
                type: "secondary"
                onClicked: {
                    if (scheduleImportViewModel.loginRequired) {
                        router.navigate("Login")
                        return
                    }
                    importDialog.open()
                    scheduleImportViewModel.loadSemesters()
                }
            }
            AppButton {
                text: "新增课程"
                enabled: scheduleViewModel.hasSchedule
                onClicked: root.openCourseForAdd(1, 1)
            }
            AppButton {
                text: "课表管理"
                type: "secondary"
                onClicked: {
                    managementDialog.scheduleList = scheduleViewModel.schedules()
                    managementDialog.open()
                }
            }
            AppButton {
                text: "设置"
                type: "secondary"
                enabled: scheduleViewModel.hasSchedule
                onClicked: root.openSettings()
            }
        }

        Card {
            Layout.fillWidth: true
            implicitHeight: 48
            visible: scheduleViewModel.hasSchedule

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                WeekSwitcher {
                    currentWeek: scheduleViewModel.currentWeek
                    totalWeeks: scheduleViewModel.totalWeeks
                    onPreviousWeek: scheduleViewModel.goPreviousWeek()
                    onNextWeek: scheduleViewModel.goNextWeek()
                    onGoToCurrentWeek: scheduleViewModel.goToCurrentWeek()
                }

                Item { Layout.fillWidth: true }

                CheckBox {
                    text: "显示非本周课程"
                    checked: scheduleViewModel.showNonCurrentWeekCourses
                    onToggled: scheduleViewModel.showNonCurrentWeekCourses = checked
                }

                AppButton {
                    text: "同步教学周"
                    type: "secondary"
                    enabled: !scheduleImportViewModel.loading
                    onClicked: {
                        if (scheduleImportViewModel.loginRequired) {
                            router.navigate("Login")
                            return
                        }
                        scheduleImportViewModel.syncCurrentWeek()
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            visible: scheduleViewModel.errorMessage.length > 0
            text: scheduleViewModel.errorMessage
            color: Theme.danger
            wrapMode: Text.WordWrap
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            LoadingView {
                anchors.fill: parent
                visible: scheduleViewModel.loading
                text: "正在加载课表..."
            }

            EmptyView {
                anchors.fill: parent
                visible: !scheduleViewModel.loading && !scheduleViewModel.hasSchedule
                title: "还没有课表"
                description: "可以新建空白课表，也可以登录后从教务系统导入。"
                actionText: "新建空白课表"
                onActionTriggered: {
                    managementDialog.scheduleList = scheduleViewModel.schedules()
                    managementDialog.open()
                }
            }

            ScrollView {
                id: gridScroll
                anchors.fill: parent
                visible: !scheduleViewModel.loading && scheduleViewModel.hasSchedule
                clip: true

                CourseGrid {
                    width: Math.max(gridScroll.availableWidth, 980)
                    height: implicitHeight
                    courseModel: scheduleViewModel.courseListModel
                    sectionsPerDay: scheduleViewModel.sectionsPerDay
                    timeSlots: scheduleViewModel.timeSlots
                    onEmptySlotClicked: function(day, section) {
                        root.openCourseForAdd(day, section)
                    }
                    onCourseClicked: function(courseId) {
                        root.openCourseForEdit(courseId)
                    }
                }
            }
        }
    }

    CourseEditDialog {
        id: courseDialog
        parent: Overlay.overlay
        sectionsPerDay: scheduleViewModel.sectionsPerDay
        onSaved: {
            courseEditViewModel.name = courseName
            courseEditViewModel.teacher = courseTeacher
            courseEditViewModel.location = courseLocation
            courseEditViewModel.startWeek = startWeek
            courseEditViewModel.endWeek = endWeek
            courseEditViewModel.dayOfWeek = dayOfWeek
            courseEditViewModel.startSection = startSection
            courseEditViewModel.endSection = endSection
            courseEditViewModel.weekType = weekType
            courseEditViewModel.colorValue = root.argbValue(courseColor)
            if (courseEditViewModel.save()) {
                courseDialog.close()
                scheduleViewModel.load()
            } else {
                courseDialog.errorText = courseEditViewModel.errorMessage
            }
        }
        onDeleteRequested: {
            pendingCourseId = courseId
            deleteCourseDialog.open()
        }
    }

    ImportScheduleDialog {
        id: importDialog
        parent: Overlay.overlay
        loading: scheduleImportViewModel.loading
        semesters: scheduleImportViewModel.semesters
        errorText: scheduleImportViewModel.errorMessage
        statusText: scheduleImportViewModel.statusMessage
        onRefreshClicked: scheduleImportViewModel.loadSemesters()
        onImportClicked: function(planCode, label) {
            scheduleImportViewModel.importSchedule(planCode, label)
        }
    }

    ScheduleManagementDialog {
        id: managementDialog
        parent: Overlay.overlay
        onSwitchSchedule: function(scheduleId) {
            scheduleViewModel.switchSchedule(scheduleId)
            scheduleList = scheduleViewModel.schedules()
        }
        onNewSchedule: function(name) {
            if (!scheduleViewModel.createSchedule(name))
                toastManager.show(scheduleViewModel.errorMessage, "error")
            scheduleList = scheduleViewModel.schedules()
        }
        onRenameSchedule: function(scheduleId, name) {
            if (!scheduleViewModel.renameSchedule(scheduleId, name))
                toastManager.show(scheduleViewModel.errorMessage, "error")
            scheduleList = scheduleViewModel.schedules()
        }
        onDeleteScheduleRequested: function(scheduleId) {
            pendingScheduleId = scheduleId
            deleteScheduleDialog.open()
        }
    }

    ScheduleSettingsDialog {
        id: settingsDialog
        parent: Overlay.overlay
        onSaveClicked: {
            var data = {
                semesterName: semesterName,
                semesterStartDate: semesterStartDate,
                totalWeeks: totalWeeks,
                morningSections: morningSections,
                afternoonSections: afternoonSections,
                eveningSections: eveningSections,
                courseDuration: courseDuration,
                breakDuration: breakDuration,
                autoSyncTime: autoSyncTime,
                timeSlotPreset: timeSlotPreset,
                timeSlots: timeSlots
            }
            if (scheduleViewModel.saveCurrentConfig(data)) {
                settingsDialog.close()
            } else {
                settingsDialog.errorText = scheduleViewModel.errorMessage
            }
        }
    }

    property string pendingCourseId: ""
    property string pendingScheduleId: ""

    AppDialog {
        id: deleteCourseDialog
        parent: Overlay.overlay
        title: "删除课程"
        message: "确定删除这门课程吗？"
        confirmText: "删除"
        type: "danger"
        onConfirmed: {
            scheduleViewModel.deleteCourse(root.pendingCourseId)
            scheduleViewModel.load()
            courseDialog.close()
        }
    }

    AppDialog {
        id: deleteScheduleDialog
        parent: Overlay.overlay
        title: "删除课表"
        message: "删除后，该课表中的课程也会一并删除。"
        confirmText: "删除"
        type: "danger"
        onConfirmed: {
            if (!scheduleViewModel.deleteSchedule(root.pendingScheduleId))
                toastManager.show(scheduleViewModel.errorMessage, "error")
            managementDialog.scheduleList = scheduleViewModel.schedules()
        }
    }

    Dialog {
        id: conflictDialog
        parent: Overlay.overlay
        title: "课表名称冲突"
        modal: true
        width: 460
        standardButtons: Dialog.NoButton

        contentItem: ColumnLayout {
            spacing: 14
            Text {
                Layout.fillWidth: true
                text: scheduleImportViewModel.conflictMessage
                color: Theme.text
                wrapMode: Text.WordWrap
            }
            RowLayout {
                Layout.fillWidth: true
                AppButton {
                    text: "取消"
                    type: "secondary"
                    onClicked: {
                        scheduleImportViewModel.resolveConflict("cancel")
                        conflictDialog.close()
                    }
                }
                Item { Layout.fillWidth: true }
                AppButton {
                    text: "添加后缀"
                    type: "secondary"
                    onClicked: {
                        scheduleImportViewModel.resolveConflict("addSuffix")
                        conflictDialog.close()
                    }
                }
                AppButton {
                    text: "更新已有课表"
                    onClicked: {
                        scheduleImportViewModel.resolveConflict("updateExisting")
                        conflictDialog.close()
                    }
                }
            }
        }
    }
}
