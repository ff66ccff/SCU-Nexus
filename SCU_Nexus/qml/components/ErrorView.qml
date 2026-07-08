import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../styles"

Item {
    id: root

    property string title: "加载失败"
    property string message: ""
    property bool canRetry: true
    property string retryText: "重试"
    signal retry()

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 40, 420)
        spacing: 12

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            width: 48
            height: 48
            radius: 24
            color: Theme.dangerSoft
            border.color: Theme.dangerBorder

            Text {
                anchors.centerIn: parent
                text: "!"
                font.pixelSize: 22
                font.bold: true
                color: Theme.danger
            }
        }

        Text {
            Layout.fillWidth: true
            text: root.title
            font.pixelSize: 18
            font.weight: Theme.weightStrong
            color: Theme.text
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        Text {
            Layout.fillWidth: true
            text: root.message
            font.pixelSize: Theme.fontLabel
            color: Theme.mutedText
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            visible: root.message.length > 0
        }

        AppButton {
            Layout.alignment: Qt.AlignHCenter
            visible: root.canRetry
            text: root.retryText
            type: "secondary"
            onClicked: root.retry()
        }
    }
}
