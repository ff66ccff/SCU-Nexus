import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: scheduleSettingsDialog
    title: "课表设置"
    width: 480
    height: 540
    modal: true

    property string semesterName: ""
    property date semesterStartDate: new Date()
    property int totalWeeks: 20
    property int morningSections: 4
    property int afternoonSections: 5
    property int eveningSections: 3
    property int courseDuration: 45
    property int breakDuration: 10
    property string timeSlotPreset: "jiangAn"  // "jiangAn", "wangJiangHuaXi", "custom"
    property bool autoSyncTime: true

    signal saveClicked()
    signal cancelClicked()

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                width: parent.width
                spacing: 10

                // Semester name
                LabeledField {
                    label: "学期名称"
                    Layout.fillWidth: true
                    TextField {
                        Layout.fillWidth: true
                        text: scheduleSettingsDialog.semesterName
                        onTextChanged: scheduleSettingsDialog.semesterName = text
                    }
                }

                // Semester start date
                LabeledField {
                    label: "开学日期"
                    Layout.fillWidth: true
                    RowLayout {
                        // Simplified date display - use TextField for now
                        TextField {
                            Layout.fillWidth: true
                            text: scheduleSettingsDialog.semesterStartDate.toISOString().slice(0, 10)
                            placeholderText: "YYYY-MM-DD"
                        }
                    }
                }

                // Total weeks
                LabeledField {
                    label: "总周数"
                    Layout.fillWidth: true
                    SpinBox {
                        Layout.fillWidth: true
                        from: 1
                        to: 30
                        value: scheduleSettingsDialog.totalWeeks
                        onValueChanged: scheduleSettingsDialog.totalWeeks = value
                    }
                }

                // Section counts
                GroupBox {
                    title: "节次配置"
                    Layout.fillWidth: true

                    RowLayout {
                        spacing: 12
                        LabeledField {
                            label: "上午节数"
                            Layout.preferredWidth: 100
                            SpinBox {
                                Layout.fillWidth: true
                                from: 1
                                to: 6
                                value: scheduleSettingsDialog.morningSections
                                onValueChanged: scheduleSettingsDialog.morningSections = value
                            }
                        }
                        LabeledField {
                            label: "下午节数"
                            Layout.preferredWidth: 100
                            SpinBox {
                                Layout.fillWidth: true
                                from: 1
                                to: 6
                                value: scheduleSettingsDialog.afternoonSections
                                onValueChanged: scheduleSettingsDialog.afternoonSections = value
                            }
                        }
                        LabeledField {
                            label: "晚上节数"
                            Layout.preferredWidth: 100
                            SpinBox {
                                Layout.fillWidth: true
                                from: 0
                                to: 4
                                value: scheduleSettingsDialog.eveningSections
                                onValueChanged: scheduleSettingsDialog.eveningSections = value
                            }
                        }
                    }
                }

                // Time settings
                RowLayout {
                    spacing: 12
                    LabeledField {
                        label: "课程时长(分钟)"
                        Layout.preferredWidth: 140
                        SpinBox {
                            Layout.fillWidth: true
                            from: 30
                            to: 60
                            stepSize: 5
                            value: scheduleSettingsDialog.courseDuration
                            onValueChanged: scheduleSettingsDialog.courseDuration = value
                        }
                    }
                    LabeledField {
                        label: "课间休息(分钟)"
                        Layout.preferredWidth: 140
                        SpinBox {
                            Layout.fillWidth: true
                            from: 5
                            to: 30
                            stepSize: 5
                            value: scheduleSettingsDialog.breakDuration
                            onValueChanged: scheduleSettingsDialog.breakDuration = value
                        }
                    }
                }

                // Time slot preset
                LabeledField {
                    label: "时间表预设"
                    Layout.fillWidth: true
                    ComboBox {
                        Layout.fillWidth: true
                        model: ["江安校区 (4-5-3)", "望江/华西 (4-5-3)", "自定义"]
                        currentIndex: {
                            switch(scheduleSettingsDialog.timeSlotPreset) {
                                case "wangJiangHuaXi": return 1
                                case "custom": return 2
                                default: return 0
                            }
                        }
                        onCurrentIndexChanged: {
                            switch(currentIndex) {
                                case 0: scheduleSettingsDialog.timeSlotPreset = "jiangAn"; break
                                case 1: scheduleSettingsDialog.timeSlotPreset = "wangJiangHuaXi"; break
                                case 2: scheduleSettingsDialog.timeSlotPreset = "custom"; break
                            }
                        }
                    }
                }

                // Auto sync
                RowLayout {
                    Label { text: "自动同步时间" }
                    Switch {
                        checked: scheduleSettingsDialog.autoSyncTime
                        onCheckedChanged: scheduleSettingsDialog.autoSyncTime = checked
                    }
                }

                // Time slot preview
                GroupBox {
                    title: "时间表预览"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 160

                    ListView {
                        anchors.fill: parent
                        clip: true
                        model: 12  // placeholder count
                        delegate: RowLayout {
                            width: parent.width
                            Label {
                                text: (index + 1) + "."
                                Layout.preferredWidth: 25
                                font.pixelSize: 12
                            }
                            Label {
                                text: "第" + (index + 1) + "节"
                                Layout.fillWidth: true
                                font.pixelSize: 12
                            }
                        }
                    }
                }
            }
        }

        // Bottom buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Item { Layout.fillWidth: true }

            Button {
                text: "取消"
                onClicked: {
                    cancelClicked()
                    scheduleSettingsDialog.close()
                }
            }
            Button {
                text: "保存"
                highlighted: true
                onClicked: {
                    saveClicked()
                    scheduleSettingsDialog.close()
                }
            }
        }
    }
}
