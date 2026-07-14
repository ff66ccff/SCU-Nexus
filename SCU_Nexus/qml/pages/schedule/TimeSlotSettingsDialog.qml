pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../styles"

Dialog {
    id: root

    title: "自定义时间表"
    width: 440
    height: 560
    modal: true

    property var timeSlots: []
    property int sectionsPerDay: 12
    property string errorText: ""
    signal applied(var newSlots)

    function rebuild() {
        slotModel.clear()
        for (var i = 0; i < sectionsPerDay; ++i) {
            var value = i < timeSlots.length ? timeSlots[i] : {startTime:"", endTime:""}
            slotModel.append({startTime: value.startTime || "", endTime: value.endTime || ""})
        }
        errorText = ""
    }

    function validTime(value) {
        return /^([01][0-9]|2[0-3]):[0-5][0-9]$/.test(value)
    }

    onOpened: rebuild()

    ListModel { id: slotModel }

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

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: slotModel
            spacing: 6

            delegate: RowLayout {
                id: slotRow

                required property int index
                required property string startTime
                required property string endTime
                width: ListView.view.width
                Label {
                    text: "第 " + (slotRow.index + 1) + " 节"
                    Layout.preferredWidth: 64
                }
                TextField {
                    Layout.fillWidth: true
                    text: slotRow.startTime
                    placeholderText: "08:15"
                    inputMask: "99:99"
                    onTextChanged: slotModel.setProperty(slotRow.index,
                                                         "startTime", text)
                }
                Label { text: "—" }
                TextField {
                    Layout.fillWidth: true
                    text: slotRow.endTime
                    placeholderText: "09:00"
                    inputMask: "99:99"
                    onTextChanged: slotModel.setProperty(slotRow.index,
                                                         "endTime", text)
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Item { Layout.fillWidth: true }
            Button { text: "取消"; onClicked: root.close() }
            Button {
                text: "应用"
                highlighted: true
                onClicked: {
                    var result = []
                    for (var i = 0; i < slotModel.count; ++i) {
                        var item = slotModel.get(i)
                        if (!root.validTime(item.startTime) || !root.validTime(item.endTime)
                                || item.startTime >= item.endTime) {
                            root.errorText = "第 " + (i + 1) + " 节的时间无效"
                            return
                        }
                        result.push({startTime:item.startTime, endTime:item.endTime})
                    }
                    root.applied(result)
                    root.close()
                }
            }
        }
    }
}
