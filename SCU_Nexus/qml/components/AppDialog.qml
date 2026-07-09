import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../styles"

Dialog {
    id: root

    property string message: ""
    property string confirmText: "确认"
    property string cancelText: "取消"
    property string type: "normal"

    signal confirmed()

    modal: true
    dim: true
    standardButtons: Dialog.NoButton
    padding: 0
    width: Math.min(parent ? parent.width - 48 : 420, 420)

    background: Rectangle {
        radius: Theme.cardRadius
        color: Theme.surface
        border.color: Theme.border
    }

    contentItem: ColumnLayout {
        spacing: 16

        Text {
            Layout.fillWidth: true
            Layout.margins: 20
            Layout.bottomMargin: 0
            text: root.title
            font.pixelSize: Theme.fontSection
            font.weight: Theme.weightStrong
            color: Theme.text
            wrapMode: Text.WordWrap
        }

        Text {
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            text: root.message
            font.pixelSize: Theme.fontLabel
            color: Theme.mutedText
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 20
            Layout.topMargin: 0
            spacing: 10

            Item { Layout.fillWidth: true }

            AppButton {
                text: root.cancelText
                type: "secondary"
                onClicked: root.close()
            }

            AppButton {
                text: root.confirmText
                type: root.type === "danger" ? "danger" : "primary"
                // 触发确认信号并关闭当前对话框。
                onClicked: {
                    root.confirmed()
                    root.close()
                }
            }
        }
    }
}
