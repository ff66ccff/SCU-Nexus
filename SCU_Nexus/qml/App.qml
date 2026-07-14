import QtQuick 2.15
import QtQuick.Controls 2.15
import "components"
import "styles"

// Context properties are intentionally injected by main.cpp.
// qmllint disable unqualified

ApplicationWindow {
    id: window

    width: 1100
    height: 760
    minimumWidth: 900
    minimumHeight: 620
    visible: true
    title: "SCU Nexus"
    color: Theme.background
    font.family: Theme.fontFamily

    // 全局把 Fluent 控件的强调色设为四川大学品牌红（按钮、开关、焦点线等都会跟随）。
    palette.accent: Theme.accent

    Component.onCompleted: {
        const geometry = appSettings.restoreWindowGeometry()
        window.x = geometry.x
        window.y = geometry.y
        window.width = geometry.width
        window.height = geometry.height
        appController.initialize()
    }

    onClosing: function(close) {
        appSettings.saveWindowGeometry(window.x, window.y, window.width, window.height)
        close.accepted = true
    }

    MainShell {
        anchors.fill: parent
        visible: appController.ready
    }

    LoadingView {
        anchors.fill: parent
        visible: !appController.ready && appController.startupError.length === 0
        text: "正在启动 SCU Nexus..."
    }

    ErrorView {
        anchors.fill: parent
        visible: appController.startupError.length > 0
        title: "启动失败"
        message: appController.startupError
        onRetry: appController.initialize()
    }
}
