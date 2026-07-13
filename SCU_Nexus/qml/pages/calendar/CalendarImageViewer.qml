import QtQuick 2.15
import QtQuick.Controls 2.15

Dialog {
    id: root

    property string imageUrl: ""

    modal: true
    width: Math.min(parent ? parent.width - 48 : 960, 980)
    height: Math.min(parent ? parent.height - 48 : 720, 760)
    title: "校历预览"
    standardButtons: Dialog.Close

    contentItem: Item {
        ScrollView {
            anchors.fill: parent
            clip: true

            Image {
                id: calendarImage

                width: Math.max(root.width - 48, 320)
                source: root.imageUrl
                fillMode: Image.PreserveAspectFit
                asynchronous: true
            }
        }

        BusyIndicator {
            anchors.centerIn: parent
            running: calendarImage.status === Image.Loading
            visible: running
        }

        Label {
            anchors.centerIn: parent
            width: Math.min(parent.width - 48, 520)
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            visible: calendarImage.status === Image.Error
            text: "校历图片加载失败，请检查网络后重试。"
        }
    }
}
