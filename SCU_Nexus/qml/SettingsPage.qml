import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "components"
import "styles"

Item {
    id: root

    property string pendingAction: ""

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: Theme.sectionGap

            ModuleHeader {
                Layout.fillWidth: true
                Layout.margins: Theme.pagePadding
                Layout.bottomMargin: 0
                title: "设置"
                subtitle: "账户、缓存、外观和版本信息。"
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.pagePadding
                Layout.rightMargin: Theme.pagePadding
                implicitHeight: accountColumn.implicitHeight + 32
                radius: Theme.cardRadius
                color: Theme.surface
                border.color: Theme.border

                ColumnLayout {
                    id: accountColumn
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    SectionHeader {
                        Layout.fillWidth: true
                        title: "账户"
                        description: appController.loggedIn ? "当前已登录统一身份认证。" : "登录后可查询考表和教务成绩。"
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        Text {
                            Layout.fillWidth: true
                            text: appController.loggedIn ? "已登录" : "未登录"
                            font.pixelSize: Theme.fontBody
                            color: Theme.text
                        }

                        AppButton {
                            text: appController.loggedIn ? "退出登录" : "去登录"
                            type: appController.loggedIn ? "danger" : "primary"
                            // 根据当前登录状态打开退出确认框或跳转登录页。
                            onClicked: {
                                if (appController.loggedIn) {
                                    pendingAction = "logout"
                                    confirmDialog.title = "退出登录"
                                    confirmDialog.message = "确定要退出当前账号吗？"
                                    confirmDialog.confirmText = "退出"
                                    confirmDialog.type = "danger"
                                    confirmDialog.open()
                                } else {
                                    router.navigate("Login")
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.pagePadding
                Layout.rightMargin: Theme.pagePadding
                implicitHeight: dataColumn.implicitHeight + 32
                radius: Theme.cardRadius
                color: Theme.surface
                border.color: Theme.border

                ColumnLayout {
                    id: dataColumn
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    SectionHeader {
                        Layout.fillWidth: true
                        title: "数据管理"
                        description: "实际清理动作由 C/D 对应 ViewModel 接入。"
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        AppButton {
                            text: "清除课表缓存"
                            type: "secondary"
                            onClicked: openDangerConfirm("clearSchedule", "清除课表缓存", "确定要清除本地课表缓存吗？")
                        }

                        AppButton {
                            text: "清除查询缓存"
                            type: "secondary"
                            onClicked: openDangerConfirm("clearQuery", "清除查询缓存", "确定要清除考表、成绩等查询缓存吗？")
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.pagePadding
                Layout.rightMargin: Theme.pagePadding
                implicitHeight: appearanceColumn.implicitHeight + 32
                radius: Theme.cardRadius
                color: Theme.surface
                border.color: Theme.border

                ColumnLayout {
                    id: appearanceColumn
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    SectionHeader {
                        Layout.fillWidth: true
                        title: "外观"
                        description: "主题设置统一由 ThemeManager 输出。"
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        AppButton { text: "跟随系统"; type: themeManager.mode === "system" ? "primary" : "secondary"; onClicked: themeManager.setMode("system") }
                        AppButton { text: "浅色"; type: themeManager.mode === "light" ? "primary" : "secondary"; onClicked: themeManager.setMode("light") }
                        AppButton { text: "深色"; type: themeManager.mode === "dark" ? "primary" : "secondary"; onClicked: themeManager.setMode("dark") }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.pagePadding
                Layout.rightMargin: Theme.pagePadding
                implicitHeight: 58
                radius: Theme.cardRadius
                color: Theme.surface
                border.color: Theme.border

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 16

                    Text {
                        text: "版本"
                        font.pixelSize: Theme.fontBody
                        color: Theme.mutedText
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "v" + appController.appVersion
                        font.pixelSize: Theme.fontBody
                        color: Theme.text
                        horizontalAlignment: Text.AlignRight
                    }
                }
            }
        }
    }

    AppDialog {
        id: confirmDialog
        anchors.centerIn: parent
        // 根据待确认动作执行退出登录或缓存清理提示。
        onConfirmed: {
            if (pendingAction === "logout") {
                appController.authViewModel.logout()
                toastManager.show("已退出登录")
            } else if (pendingAction === "clearSchedule") {
                toastManager.show("清除课表缓存接口已预留")
            } else if (pendingAction === "clearQuery") {
                toastManager.show("清除查询缓存接口已预留")
            }
            pendingAction = ""
        }
    }

    // 打开危险操作确认框，并暂存用户即将执行的动作。
    function openDangerConfirm(action, title, message) {
        pendingAction = action
        confirmDialog.title = title
        confirmDialog.message = message
        confirmDialog.confirmText = "清除"
        confirmDialog.type = "danger"
        confirmDialog.open()
    }
}
