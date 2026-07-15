import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"
import "../../styles"

Dialog {
    id: root

    property string selectedDate: ""
    signal dateAccepted(string date)

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    title: "选择查询日期"
    standardButtons: Dialog.NoButton
    width: Math.min(parent ? parent.width - 48 : 420, 420)

    contentItem: ColumnLayout {
        spacing: Theme.spacing16

        Text {
            Layout.fillWidth: true
            text: "可查询今天前 7 天至今天后 30 天，请输入 YYYY-MM-DD。"
            font.pixelSize: Theme.fontLabel
            color: Theme.mutedText
            wrapMode: Text.WordWrap
        }

        AppTextField {
            id: dateField
            Layout.fillWidth: true
            text: root.selectedDate
            placeholderText: "YYYY-MM-DD"
        }

        RowLayout {
            Layout.fillWidth: true
            Item { Layout.fillWidth: true }
            AppButton {
                text: "取消"
                type: "secondary"
                onClicked: root.close()
            }
            AppButton {
                text: "确定"
                onClicked: {
                    root.dateAccepted(dateField.text.trim())
                    root.close()
                }
            }
        }
    }
}
