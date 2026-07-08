import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../styles"

Item {
    id: root

    property string message: "请先登录后查看"
    signal loginRequested()

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 40, 360)
        spacing: 12

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            width: 48
            height: 48
            radius: 24
            color: Theme.control
            border.color: Theme.border

            Text {
                anchors.centerIn: parent
                text: "登录"
                font.pixelSize: Theme.fontLabel
                font.weight: Theme.weightStrong
                color: Theme.primary
            }
        }

        Text {
            Layout.fillWidth: true
            text: root.message
            font.pixelSize: Theme.fontSection
            font.weight: Theme.weightStrong
            color: Theme.text
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        AppButton {
            Layout.alignment: Qt.AlignHCenter
            text: "去登录"
            onClicked: root.loginRequested()
        }
    }
}
