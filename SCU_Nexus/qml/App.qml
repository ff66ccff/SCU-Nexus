import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "components"
import "styles"

ApplicationWindow {
    id: window

    width: 1100
    height: 760
    minimumWidth: 900
    minimumHeight: 620
    visible: true
    title: "SCU Nexus"
    color: Theme.background

    Component.onCompleted: appController.initialize()

    MainShell {
        anchors.fill: parent
        visible: appController.ready
    }

    LoadingView {
        anchors.fill: parent
        visible: !appController.ready
        text: "正在启动 SCU Nexus..."
    }
}
