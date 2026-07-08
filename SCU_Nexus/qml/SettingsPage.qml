import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    color: "transparent"

    ScrollView {
        anchors.fill: parent
        anchors.margins: 20
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: 16

            Text {
                text: "设置"
                font.pixelSize: 20
                font.bold: true
                color: "#333333"
            }

            // ========== 登录状态卡片 ==========
            GroupBox {
                title: "账户"
                Layout.fillWidth: true

                RowLayout {
                    width: parent.width

                    ColumnLayout {
                        spacing: 4
                        Layout.fillWidth: true

                        Text {
                            text: appController.loggedIn ? "已登录" : "未登录"
                            font.pixelSize: 14
                            color: appController.loggedIn ? "#4CAF50" : "#999999"
                        }

                        Text {
                            text: appController.loggedIn ? "点击退出登录" : "点击登录账号"
                            font.pixelSize: 12
                            color: "#999999"
                            visible: true
                        }
                    }

                    Button {
                        text: appController.loggedIn ? "退出登录" : "去登录"
                        onClicked: {
                            if (appController.loggedIn) {
                                appController.setLoginState(false)
                            } else {
                                router.navigate("Login")
                            }
                        }
                    }
                }
            }

            // ========== 数据管理 ==========
            GroupBox {
                title: "数据管理"
                Layout.fillWidth: true

                ColumnLayout {
                    width: parent.width
                    spacing: 8

                    Button {
                        text: "清除课表缓存"
                        Layout.fillWidth: true
                        onClicked: {
                            console.log("清除课表缓存")
                        }
                    }

                    Button {
                        text: "清除查询缓存"
                        Layout.fillWidth: true
                        onClicked: {
                            console.log("清除查询缓存")
                        }
                    }
                }
            }

            // ========== 主题设置 ==========
            GroupBox {
                title: "外观"
                Layout.fillWidth: true

                RowLayout {
                    width: parent.width
                    spacing: 8

                    Button {
                        text: "浅色"
                        Layout.fillWidth: true
                        onClicked: {
                            console.log("切换到浅色主题")
                        }
                    }

                    Button {
                        text: "深色"
                        Layout.fillWidth: true
                        onClicked: {
                            console.log("切换到深色主题")
                        }
                    }

                    Button {
                        text: "跟随系统"
                        Layout.fillWidth: true
                        onClicked: {
                            console.log("跟随系统主题")
                        }
                    }
                }
            }

            // ========== 关于 ==========
            GroupBox {
                title: "关于"
                Layout.fillWidth: true

                RowLayout {
                    width: parent.width

                    Text {
                        text: "版本"
                        font.pixelSize: 14
                        color: "#666666"
                    }

                    Text {
                        text: "v0.1.0"
                        font.pixelSize: 14
                        color: "#333333"
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
}