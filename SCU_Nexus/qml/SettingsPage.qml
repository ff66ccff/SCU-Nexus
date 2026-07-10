import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "components"
import "styles"

// 设置页：内容居中收拢为一栏（黄金分割比例控制阅读宽度与竖向节奏），
// 每个分区为带投影的 Fluent 卡片，主题切换用分段控件。
Item {
    id: root

    property string pendingAction: ""

    // 黄金分割：内容最大宽度与竖向间距按 φ≈1.618 推导，视觉更协调。
    readonly property real phi: 1.618
    readonly property int contentMaxWidth: 720
    readonly property int sectionSpacing: 20
    readonly property int innerSpacing: Math.round(sectionSpacing / phi)   // ≈ 12
    readonly property int cardMargin: 20

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: Math.min(parent.width - 2 * Theme.pagePadding, root.contentMaxWidth)
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: Math.round(root.sectionSpacing * root.phi)   // ≈ 32
            anchors.bottomMargin: root.sectionSpacing
            spacing: root.sectionSpacing

            ModuleHeader {
                Layout.fillWidth: true
                Layout.bottomMargin: Math.round(root.innerSpacing / root.phi)  // ≈ 7
                title: "设置"
                subtitle: "账户、缓存、外观和版本信息。"
            }

            // ---- 账户 ----
            Card {
                Layout.fillWidth: true
                implicitHeight: accountCol.implicitHeight + 2 * root.cardMargin

                ColumnLayout {
                    id: accountCol
                    anchors.fill: parent
                    anchors.margins: root.cardMargin
                    spacing: root.innerSpacing

                    SectionHeader {
                        Layout.fillWidth: true
                        title: "账户"
                        description: appController.loggedIn ? "当前已登录统一身份认证。" : "登录后可查询考表和教务成绩。"
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: root.innerSpacing

                        Rectangle {
                            width: 9
                            height: 9
                            radius: 4.5
                            Layout.alignment: Qt.AlignVCenter
                            color: appController.loggedIn ? Theme.success : Theme.placeholder
                        }

                        Text {
                            Layout.fillWidth: true
                            text: appController.loggedIn ? "已登录" : "未登录"
                            font.pixelSize: Theme.fontBody
                            color: Theme.text
                            verticalAlignment: Text.AlignVCenter
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

            // ---- 数据管理 ----
            Card {
                Layout.fillWidth: true
                implicitHeight: dataCol.implicitHeight + 2 * root.cardMargin

                ColumnLayout {
                    id: dataCol
                    anchors.fill: parent
                    anchors.margins: root.cardMargin
                    spacing: root.innerSpacing

                    SectionHeader {
                        Layout.fillWidth: true
                        title: "数据管理"
                        description: "实际清理动作由 C/D 对应 ViewModel 接入。"
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: root.innerSpacing

                        AppButton {
                            Layout.fillWidth: true
                            text: "清除课表缓存"
                            type: "secondary"
                            onClicked: openDangerConfirm("clearSchedule", "清除课表缓存", "确定要清除本地课表缓存吗？")
                        }

                        AppButton {
                            Layout.fillWidth: true
                            text: "清除查询缓存"
                            type: "secondary"
                            onClicked: openDangerConfirm("clearQuery", "清除查询缓存", "确定要清除考表、成绩等查询缓存吗？")
                        }
                    }
                }
            }

            // ---- 外观 ----
            Card {
                Layout.fillWidth: true
                implicitHeight: appearanceCol.implicitHeight + 2 * root.cardMargin

                ColumnLayout {
                    id: appearanceCol
                    anchors.fill: parent
                    anchors.margins: root.cardMargin
                    spacing: root.innerSpacing

                    SectionHeader {
                        Layout.fillWidth: true
                        title: "外观"
                        description: "主题设置统一由 ThemeManager 输出。"
                    }

                    SegmentedControl {
                        Layout.fillWidth: true
                        model: [
                            { label: "跟随系统", value: "system" },
                            { label: "浅色", value: "light" },
                            { label: "深色", value: "dark" }
                        ]
                        value: themeManager.mode
                        onActivated: themeManager.setMode(newValue)
                    }
                }
            }

            // ---- 版本 ----
            Card {
                Layout.fillWidth: true
                implicitHeight: 56

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: root.cardMargin
                    anchors.rightMargin: root.cardMargin

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
                if (scheduleViewModel.clearAllCourseData())
                    toastManager.show("已清除本地课表数据")
                else
                    toastManager.show(scheduleViewModel.errorMessage, "error")
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
