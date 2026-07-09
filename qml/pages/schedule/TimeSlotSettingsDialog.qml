import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: timeSlotSettingsDialog
    title: "自定义时间表"
    width: 500
    height: 500
    modal: true

    property var timeSlots: []  // Array of {startTime, endTime}
    property int sectionsPerDay: 12

    signal timeSlotsChanged(var newSlots)

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Label {
            text: "编辑每节课的开始和结束时间："
            font.pixelSize: 13
            color: "#666"
        }

        ListView {
            id: slotList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: sectionsPerDay

            delegate: RowLayout {
                width: slotList.width
                height: 40
                spacing: 8

                Label {
                    text: (index + 1) + "."
                    Layout.preferredWidth: 30
                    font.pixelSize: 13
                }

                TextField {
                    Layout.preferredWidth: 80
                    placeholderText: "HH:MM"
                    font.pixelSize: 12
                }

                Label { text: "—" }

                TextField {
                    Layout.preferredWidth: 80
                    placeholderText: "HH:MM"
                    font.pixelSize: 12
                }

                ToolButton {
                    text: "↺"
                    font.pixelSize: 14
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: "重置为江安时间表"
                onClicked: {
                    // Reset to Jiang'an defaults
                }
            }

            Button {
                text: "重置为望江/华西时间表"
                onClicked: {
                    // Reset to Wangjiang defaults
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "取消"
                onClicked: timeSlotSettingsDialog.close()
            }

            Button {
                text: "保存"
                highlighted: true
                onClicked: {
                    // Save time slots
                    timeSlotSettingsDialog.close()
                }
            }
        }
    }
}
