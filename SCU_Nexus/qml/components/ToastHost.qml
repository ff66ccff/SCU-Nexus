import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../styles"

// toastManager is intentionally injected by main.cpp; model roles are delegate-scoped.
// qmllint disable unqualified

Item {
    id: root

    anchors.right: parent.right
    anchors.bottom: parent.bottom
    anchors.margins: 20
    width: 320
    height: toastColumn.implicitHeight
    z: 100

    ListModel { id: toastModel }

    Connections {
        target: toastManager
        // 接收全局提示事件并加入临时消息队列。
        function onToastRequested(message, level) {
            toastModel.append({ message: message, level: level })
            cleanupTimer.restart()
        }
    }

    Timer {
        id: cleanupTimer
        interval: 3200
        // 定时移除最早的提示，并在队列未空时继续计时。
        onTriggered: {
            if (toastModel.count > 0) {
                toastModel.remove(0)
            }
            if (toastModel.count > 0) restart()
        }
    }

    ColumnLayout {
        id: toastColumn
        anchors.fill: parent
        spacing: 8

        Repeater {
            model: toastModel
            delegate: Rectangle {
                Layout.fillWidth: true
                implicitHeight: Math.max(44, toastText.implicitHeight + 20)
                radius: Theme.cardRadius
                color: Theme.surface
                border.color: Theme.border

                // Fluent InfoBar 风格：左侧按级别着色的圆角强调条。
                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.margins: 4
                    width: 4
                    radius: 2
                    color: model.level === "error" ? Theme.danger
                         : model.level === "warning" ? Theme.accent
                         : model.level === "success" ? Theme.success
                         : Theme.mutedText
                }

                Text {
                    id: toastText
                    anchors.fill: parent
                    anchors.leftMargin: 18
                    anchors.rightMargin: 12
                    anchors.topMargin: 10
                    anchors.bottomMargin: 10
                    text: model.message
                    font.pixelSize: Theme.fontLabel
                    color: Theme.text
                    wrapMode: Text.WordWrap
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }
}
