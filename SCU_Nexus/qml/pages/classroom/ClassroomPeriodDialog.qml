import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"
import "../../styles"

Dialog {
    id: root

    property int periodStart: 1
    property int periodEnd: 1
    signal rangeAccepted(int periodStart, int periodEnd)

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    title: "筛选空闲节次"
    standardButtons: Dialog.NoButton
    width: Math.min(parent ? parent.width - 48 : 420, 420)

    contentItem: ColumnLayout {
        spacing: Theme.spacing16

        Text {
            Layout.fillWidth: true
            text: "仅显示在整个所选节次范围内均为空闲的教室。"
            font.pixelSize: Theme.fontLabel
            color: Theme.mutedText
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacing12

            Text { text: "起始节次"; color: Theme.text }
            ComboBox {
                id: startBox
                Layout.fillWidth: true
                model: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]
                currentIndex: Math.max(0, root.periodStart - 1)
            }
            Text { text: "结束节次"; color: Theme.text }
            ComboBox {
                id: endBox
                Layout.fillWidth: true
                model: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]
                currentIndex: Math.max(0, root.periodEnd - 1)
            }
        }

        Text {
            Layout.fillWidth: true
            visible: startBox.currentIndex > endBox.currentIndex
            text: "结束节次不能早于起始节次"
            font.pixelSize: Theme.fontCaption
            color: Theme.danger
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
                text: "应用"
                enabled: startBox.currentIndex <= endBox.currentIndex
                onClicked: {
                    root.rangeAccepted(startBox.currentIndex + 1, endBox.currentIndex + 1)
                    root.close()
                }
            }
        }
    }
}
