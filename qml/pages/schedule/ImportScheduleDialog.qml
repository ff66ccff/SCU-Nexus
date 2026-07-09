import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: importScheduleDialog
    title: "从教务系统导入课表"
    width: 520
    height: 480
    modal: true

    property bool loading: false
    property string statusText: ""
    property string errorText: ""
    property var semesters: []  // List of {planCode, label, isCurrent}
    property int selectedIndex: -1

    signal importClicked(string planCode, string label)
    signal refreshClicked()

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        // Status
        Label {
            text: loading ? "正在加载学期列表..." : statusText
            font.pixelSize: 13
            color: loading ? "#1976D2" : "#333"
            visible: text !== ""
        }

        // Error
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            color: "#ffebee"
            visible: errorText !== ""
            radius: 4

            Label {
                anchors.centerIn: parent
                text: errorText
                color: "#c62828"
                font.pixelSize: 12
            }
        }

        // Semester list
        Label {
            text: "选择学期："
            font.pixelSize: 14
            font.bold: true
        }

        ListView {
            id: semesterList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: semesters

            delegate: Rectangle {
                width: semesterList.width
                height: 48
                color: index === selectedIndex ? "#e3f2fd" : (index % 2 === 0 ? "#fafafa" : "#ffffff")
                border.color: "#e0e0e0"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    Label {
                        text: modelData.label
                        font.pixelSize: 14
                        Layout.fillWidth: true
                    }

                    Label {
                        text: modelData.isCurrent ? "（当前）" : ""
                        font.pixelSize: 12
                        color: "#4CAF50"
                        visible: modelData.isCurrent
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: selectedIndex = index
                }
            }
        }

        // Action buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: "刷新列表"
                onClicked: refreshClicked()
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "取消"
                onClicked: importScheduleDialog.close()
            }

            Button {
                text: "导入所选学期"
                highlighted: true
                enabled: selectedIndex >= 0 && !loading
                onClicked: {
                    if (selectedIndex >= 0 && selectedIndex < semesters.length) {
                        var s = semesters[selectedIndex]
                        importClicked(s.planCode, s.label)
                    }
                }
            }
        }
    }

    // Conflict resolution popup
    Dialog {
        id: conflictDialog
        title: "课表名称冲突"
        width: 400
        height: 220
        modal: true

        property string conflictMessage: ""

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            Label {
                text: conflictDialog.conflictMessage
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Button {
                    text: "取消导入"
                    onClicked: conflictDialog.close()
                }
                Button {
                    text: "添加后缀"
                    onClicked: {
                        // resolveConflict("addSuffix")
                        conflictDialog.close()
                        importScheduleDialog.close()
                    }
                }
                Button {
                    text: "更新已有课表"
                    highlighted: true
                    onClicked: {
                        // resolveConflict("updateExisting")
                        conflictDialog.close()
                        importScheduleDialog.close()
                    }
                }
            }
        }
    }
}
