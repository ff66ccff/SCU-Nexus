pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "components"
import "styles"

// Context properties are intentionally injected by main.cpp.
// qmllint disable unqualified

Item {
    id: root

    property string pendingAction: ""

    readonly property int contentMaxWidth: 840
    readonly property int cardMargin: Theme.spacing20

    Flickable {
        id: settingsFlickable
        anchors.fill: parent
        contentWidth: width
        contentHeight: settingsColumn.implicitHeight + 2 * Theme.spacing24
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { }

        ColumnLayout {
            id: settingsColumn
            width: Math.max(320, Math.min(settingsFlickable.width
                                         - 2 * Theme.pagePadding,
                                         root.contentMaxWidth))
            x: Math.max(Theme.pagePadding,
                        (settingsFlickable.width - width) / 2)
            y: Theme.spacing24
            spacing: Theme.spacing16

            ModuleHeader {
                Layout.fillWidth: true
                Layout.bottomMargin: Theme.spacing4
                title: "设置"
                subtitle: "管理账户、数据、外观和应用信息"
            }

            Card {
                Layout.fillWidth: true
                implicitHeight: accountLayout.implicitHeight + 2 * root.cardMargin

                RowLayout {
                    id: accountLayout
                    anchors.fill: parent
                    anchors.margins: root.cardMargin
                    spacing: Theme.spacing16

                    Rectangle {
                        Layout.preferredWidth: 44
                        Layout.preferredHeight: 44
                        Layout.alignment: Qt.AlignVCenter
                        radius: 22
                        color: Theme.primarySoft
                        border.color: Theme.accent

                        Text {
                            anchors.centerIn: parent
                            text: appController.loggedIn ? "✓" : "人"
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSection
                            font.weight: Theme.weightStrong
                            color: Theme.accent
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacing4

                        Text {
                            Layout.fillWidth: true
                            text: "账户与安全"
                            font.pixelSize: Theme.fontSection
                            font.weight: Theme.weightStrong
                            color: Theme.text
                            elide: Text.ElideRight
                        }

                        Text {
                            Layout.fillWidth: true
                            text: appController.loggedIn
                                  ? "已登录统一身份认证，可使用教务查询与课表导入"
                                  : "当前未登录，登录后可使用教务查询与课表导入"
                            font.pixelSize: Theme.fontCaption
                            color: Theme.mutedText
                            wrapMode: Text.WordWrap
                        }
                    }

                    AppButton {
                        text: appController.loggedIn ? "退出登录" : "去登录"
                        type: appController.loggedIn ? "danger" : "primary"
                        onClicked: {
                            if (appController.loggedIn) {
                                root.pendingAction = "logout"
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

            Text {
                Layout.fillWidth: true
                Layout.topMargin: Theme.spacing8
                text: "数据管理"
                font.pixelSize: Theme.fontLabel
                font.weight: Theme.weightStrong
                color: Theme.mutedText
            }

            Card {
                Layout.fillWidth: true
                implicitHeight: dataLayout.implicitHeight + 2 * root.cardMargin

                ColumnLayout {
                    id: dataLayout
                    anchors.fill: parent
                    anchors.margins: root.cardMargin
                    spacing: Theme.spacing16

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacing16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Theme.spacing4

                            Text {
                                text: "清除课表缓存"
                                font.pixelSize: Theme.fontBody
                                font.weight: Theme.weightMedium
                                color: Theme.text
                            }

                            Text {
                                Layout.fillWidth: true
                                text: "删除本地课表、课程和课表设置数据"
                                font.pixelSize: Theme.fontCaption
                                color: Theme.mutedText
                                wrapMode: Text.WordWrap
                            }
                        }

                        AppButton {
                            text: "清除"
                            type: "secondary"
                            onClicked: root.openDangerConfirm(
                                           "clearSchedule",
                                           "清除课表缓存",
                                           "确定要清除本地课表缓存吗？")
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: Theme.border
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacing16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Theme.spacing4

                            Text {
                                text: "清除查询缓存"
                                font.pixelSize: Theme.fontBody
                                font.weight: Theme.weightMedium
                                color: Theme.text
                            }

                            Text {
                                Layout.fillWidth: true
                                text: "删除校历、考表和成绩的本地查询缓存"
                                font.pixelSize: Theme.fontCaption
                                color: Theme.mutedText
                                wrapMode: Text.WordWrap
                            }
                        }

                        AppButton {
                            text: "清除"
                            type: "secondary"
                            onClicked: root.openDangerConfirm(
                                           "clearQuery",
                                           "清除查询缓存",
                                           "确定要清除校历、考表和成绩查询缓存吗？")
                        }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                Layout.topMargin: Theme.spacing8
                text: "外观"
                font.pixelSize: Theme.fontLabel
                font.weight: Theme.weightStrong
                color: Theme.mutedText
            }

            Card {
                Layout.fillWidth: true
                implicitHeight: appearanceLayout.implicitHeight + 2 * root.cardMargin

                ColumnLayout {
                    id: appearanceLayout
                    anchors.fill: parent
                    anchors.margins: root.cardMargin
                    spacing: Theme.spacing12

                    SectionHeader {
                        Layout.fillWidth: true
                        title: "应用主题"
                        description: "主题选择会立即应用，并在下次启动时保留"
                    }

                    SegmentedControl {
                        Layout.fillWidth: true
                        Layout.maximumWidth: 480
                        model: [
                            { label: "跟随系统", value: "system" },
                            { label: "浅色", value: "light" },
                            { label: "深色", value: "dark" }
                        ]
                        value: themeManager.mode
                        onActivated: function(newValue) {
                            themeManager.setMode(newValue)
                        }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                Layout.topMargin: Theme.spacing8
                text: "关于"
                font.pixelSize: Theme.fontLabel
                font.weight: Theme.weightStrong
                color: Theme.mutedText
            }

            Card {
                Layout.fillWidth: true
                implicitHeight: aboutLayout.implicitHeight + 2 * root.cardMargin

                RowLayout {
                    id: aboutLayout
                    anchors.fill: parent
                    anchors.margins: root.cardMargin
                    spacing: Theme.spacing16

                    Rectangle {
                        Layout.preferredWidth: 44
                        Layout.preferredHeight: 44
                        radius: Theme.cardRadius
                        color: Theme.accent

                        Text {
                            anchors.centerIn: parent
                            text: "川"
                            font.pixelSize: Theme.fontSection
                            font.weight: Theme.weightStrong
                            color: Theme.accentText
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacing4

                        Text {
                            Layout.fillWidth: true
                            text: "SCU Nexus 校园学习工作台"
                            font.pixelSize: Theme.fontBody
                            font.weight: Theme.weightStrong
                            color: Theme.text
                            elide: Text.ElideRight
                        }

                        Text {
                            Layout.fillWidth: true
                            text: "版本 v" + appController.appVersion
                            font.pixelSize: Theme.fontCaption
                            color: Theme.mutedText
                        }
                    }
                }
            }
        }
    }

    AppDialog {
        id: confirmDialog
        anchors.centerIn: parent

        onConfirmed: {
            if (root.pendingAction === "logout") {
                appController.authViewModel.logout()
                toastManager.show("已退出登录")
            } else if (root.pendingAction === "clearSchedule") {
                if (scheduleViewModel.clearAllCourseData())
                    toastManager.show("已清除本地课表数据")
                else
                    toastManager.show(scheduleViewModel.errorMessage, "error")
            } else if (root.pendingAction === "clearQuery") {
                if (queryCacheViewModel.clearAll())
                    toastManager.show("已清除考表、成绩和校历查询缓存", "success")
                else
                    toastManager.show(queryCacheViewModel.errorMessage, "error")
            }
            root.pendingAction = ""
        }
    }

    function openDangerConfirm(action, dialogTitle, dialogMessage) {
        root.pendingAction = action
        confirmDialog.title = dialogTitle
        confirmDialog.message = dialogMessage
        confirmDialog.confirmText = "清除"
        confirmDialog.type = "danger"
        confirmDialog.open()
    }
}
