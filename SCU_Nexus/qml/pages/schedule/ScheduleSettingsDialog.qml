pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../styles"

Dialog {
    id: root

    title: "课表设置"
    width: 520
    height: 620
    modal: true

    property string semesterName: ""
    property string semesterStartDate: ""
    property int totalWeeks: 20
    property int morningSections: 4
    property int afternoonSections: 5
    property int eveningSections: 3
    property int courseDuration: 45
    property int breakDuration: 10
    property string timeSlotPreset: "jiangAn"
    property bool autoSyncTime: true
    property var timeSlots: []
    property string errorText: ""
    readonly property int sectionsPerDay: morningSections + afternoonSections + eveningSections

    signal saveClicked()
    signal cancelClicked()

    function jiangAnSlots() {
        return [
            {startTime:"08:15", endTime:"09:00"}, {startTime:"09:10", endTime:"09:55"},
            {startTime:"10:15", endTime:"11:00"}, {startTime:"11:10", endTime:"11:55"},
            {startTime:"13:50", endTime:"14:35"}, {startTime:"14:45", endTime:"15:30"},
            {startTime:"15:40", endTime:"16:25"}, {startTime:"16:45", endTime:"17:30"},
            {startTime:"17:40", endTime:"18:25"}, {startTime:"19:20", endTime:"20:05"},
            {startTime:"20:15", endTime:"21:00"}, {startTime:"21:10", endTime:"21:55"}
        ]
    }

    function wangJiangSlots() {
        return [
            {startTime:"08:00", endTime:"08:45"}, {startTime:"08:55", endTime:"09:40"},
            {startTime:"10:00", endTime:"10:45"}, {startTime:"10:55", endTime:"11:40"},
            {startTime:"14:00", endTime:"14:45"}, {startTime:"14:55", endTime:"15:40"},
            {startTime:"15:50", endTime:"16:35"}, {startTime:"16:55", endTime:"17:40"},
            {startTime:"17:50", endTime:"18:35"}, {startTime:"19:30", endTime:"20:15"},
            {startTime:"20:25", endTime:"21:10"}, {startTime:"21:20", endTime:"22:05"}
        ]
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Text {
            Layout.fillWidth: true
            visible: root.errorText.length > 0
            text: root.errorText
            color: Theme.danger
            wrapMode: Text.WordWrap
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth

            ColumnLayout {
                width: parent.width
                spacing: 10

                Label { text: "学期名称" }
                TextField {
                    Layout.fillWidth: true
                    text: root.semesterName
                    onTextChanged: root.semesterName = text
                }

                Label { text: "开学日期（YYYY-MM-DD）" }
                TextField {
                    Layout.fillWidth: true
                    text: root.semesterStartDate
                    placeholderText: "2026-02-23"
                    onTextChanged: root.semesterStartDate = text
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    ColumnLayout {
                        Label { text: "总周数" }
                        SpinBox {
                            from: 1; to: 30
                            value: root.totalWeeks
                            onValueChanged: root.totalWeeks = value
                        }
                    }
                    ColumnLayout {
                        Label { text: "上午节数" }
                        SpinBox {
                            from: 1; to: 6
                            value: root.morningSections
                            onValueChanged: root.morningSections = value
                        }
                    }
                    ColumnLayout {
                        Label { text: "下午节数" }
                        SpinBox {
                            from: 1; to: 6
                            value: root.afternoonSections
                            onValueChanged: root.afternoonSections = value
                        }
                    }
                    ColumnLayout {
                        Label { text: "晚上节数" }
                        SpinBox {
                            from: 0; to: 4
                            value: root.eveningSections
                            onValueChanged: root.eveningSections = value
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12
                    ColumnLayout {
                        Label { text: "课程时长（分钟）" }
                        SpinBox {
                            from: 30; to: 90; stepSize: 5
                            value: root.courseDuration
                            onValueChanged: root.courseDuration = value
                        }
                    }
                    ColumnLayout {
                        Label { text: "课间休息（分钟）" }
                        SpinBox {
                            from: 0; to: 30; stepSize: 5
                            value: root.breakDuration
                            onValueChanged: root.breakDuration = value
                        }
                    }
                    Item { Layout.fillWidth: true }
                    CheckBox {
                        text: "自动同步时间"
                        checked: root.autoSyncTime
                        onToggled: root.autoSyncTime = checked
                    }
                }

                Label { text: "时间表预设" }
                RowLayout {
                    Layout.fillWidth: true
                    ComboBox {
                        id: presetBox
                        Layout.fillWidth: true
                        model: ["江安校区", "望江/华西", "自定义"]
                        currentIndex: root.timeSlotPreset === "wangJiangHuaXi" ? 1
                                      : (root.timeSlotPreset === "custom" ? 2 : 0)
                        onActivated: {
                            if (currentIndex === 0) {
                                root.timeSlotPreset = "jiangAn"
                                root.timeSlots = root.jiangAnSlots()
                            } else if (currentIndex === 1) {
                                root.timeSlotPreset = "wangJiangHuaXi"
                                root.timeSlots = root.wangJiangSlots()
                            } else {
                                root.timeSlotPreset = "custom"
                            }
                        }
                    }
                    Button {
                        text: "编辑时间"
                        visible: root.timeSlotPreset === "custom"
                        onClicked: {
                            timeEditor.timeSlots = root.timeSlots
                            timeEditor.sectionsPerDay = root.sectionsPerDay
                            timeEditor.open()
                        }
                    }
                }

                GroupBox {
                    title: "时间表预览"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 190

                    ListView {
                        anchors.fill: parent
                        clip: true
                        model: root.timeSlots
                        delegate: RowLayout {
                            required property int index
                            required property var modelData
                            width: ListView.view.width
                            Label { text: (index + 1) + "."; Layout.preferredWidth: 28 }
                            Label {
                                text: modelData.startTime + " — " + modelData.endTime
                                Layout.fillWidth: true
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Item { Layout.fillWidth: true }
            Button {
                text: "取消"
                onClicked: {
                    root.cancelClicked()
                    root.close()
                }
            }
            Button {
                text: "保存"
                highlighted: true
                onClicked: root.saveClicked()
            }
        }
    }

    TimeSlotSettingsDialog {
        id: timeEditor
        parent: Overlay.overlay
        onApplied: function(newSlots) {
            root.timeSlots = newSlots
            root.timeSlotPreset = "custom"
        }
    }
}
