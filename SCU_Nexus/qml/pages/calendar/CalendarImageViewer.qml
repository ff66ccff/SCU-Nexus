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

    contentItem: ScrollView {
        clip: true

        Image {
            width: Math.max(root.width - 48, 320)
            source: root.imageUrl
            fillMode: Image.PreserveAspectFit
            asynchronous: true
        }
    }
}
