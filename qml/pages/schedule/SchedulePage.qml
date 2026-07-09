import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// Main schedule page - weekly view with course grid

Page {
    id: schedulePage

    property int displayWeek: 1
    property int totalWeeks: 20
    property string scheduleName: ""
    property bool hasSchedule: false

    // ViewModel bindings set from C++
    // scheduleViewModel, courseEditViewModel, importViewModel, etc.

    background: Rectangle { color: "#f5f5f5" }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ---- Top Bar ----
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: "#ffffff"
            border.color: "#e0e0e0"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 12

                // Schedule name
                Label {
                    text: schedulePage.hasSchedule ? schedulePage.scheduleName : "无课表"
                    font.pixelSize: 18
                    font.bold: true
                    Layout.fillWidth: true
                }

                // Week navigator
                RowLayout {
                    spacing: 4

                    ToolButton {
                        text: "◀"
                        onClicked: { /* scheduleViewModel.goPreviousWeek() */ }
                    }

                    Label {
                        text: "第 " + schedulePage.displayWeek + " 周 / 共 " + schedulePage.totalWeeks + " 周"
                        Layout.preferredWidth: 180
                        horizontalAlignment: Text.AlignHCenter
                        font.pixelSize: 14
                    }

                    ToolButton {
                        text: "▶"
                        onClicked: { /* scheduleViewModel.goNextWeek() */ }
                    }

                    ToolButton {
                        text: "今"
                        font.bold: true
                        onClicked: { /* scheduleViewModel.goToCurrentWeek() */ }
                    }
                }

                // Action buttons
                RowLayout {
                    spacing: 6

                    Button {
                        text: "导入课表"
                        onClicked: importDialog.open()
                    }

                    Button {
                        text: "新建课程"
                        onClicked: courseEditDialog.openForAdd()
                    }

                    Button {
                        text: "课表管理"
                        onClicked: scheduleMgmtDialog.open()
                    }

                    Button {
                        text: "设置"
                        onClicked: scheduleSettingsDialog.open()
                    }
                }
            }
        }

        // ---- Main Content ----
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Empty state
            Rectangle {
                anchors.centerIn: parent
                visible: !schedulePage.hasSchedule
                color: "transparent"
                width: 300
                height: 200

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 16

                    Label {
                        text: "📚"
                        font.pixelSize: 48
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Label {
                        text: "还没有课程表"
                        font.pixelSize: 16
                        color: "#999"
                        Layout.alignment: Qt.AlignHCenter
                    }
                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 12

                        Button {
                            text: "从教务系统导入"
                            onClicked: importDialog.open()
                        }
                        Button {
                            text: "新建空白课表"
                            onClicked: {
                                // scheduleViewModel creates new schedule via management dialog
                                scheduleMgmtDialog.open()
                            }
                        }
                    }
                }
            }

            // Week grid (visible when hasSchedule)
            CourseGrid {
                anchors.fill: parent
                anchors.margins: 8
                visible: schedulePage.hasSchedule
                displayWeek: schedulePage.displayWeek
            }
        }
    }

    // Dialogs
    Dialog {
        id: importDialog
        title: "导入课表"
        width: 500
        height: 400
        modal: true
        // ImportScheduleDialog content would go here
    }

    Dialog {
        id: courseEditDialog
        title: "编辑课程"
        width: 480
        height: 560
        modal: true
        // CourseEditDialog content would go here
    }

    Dialog {
        id: scheduleMgmtDialog
        title: "课表管理"
        width: 500
        height: 400
        modal: true
        // ScheduleManagementDialog content would go here
    }

    Dialog {
        id: scheduleSettingsDialog
        title: "课表设置"
        width: 480
        height: 520
        modal: true
        // ScheduleSettingsDialog content would go here
    }
}
