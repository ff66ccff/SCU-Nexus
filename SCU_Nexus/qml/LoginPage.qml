import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "components"

Rectangle {
    id: root
    color: "#f5f5f5"

    ColumnLayout {
        anchors.centerIn: parent
        width: 360
        spacing: 16

        Text {
            text: "SCU Nexus"
            font.pixelSize: 28
            font.bold: true
            color: "#1976d2"
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 12
        }

        Rectangle {
            Layout.fillWidth: true
            height: 40
            color: "#ffebee"
            radius: 4
            visible: false

            Text {
                anchors.centerIn: parent
                text: "登录失败，请检查账号密码"
                font.pixelSize: 13
                color: "#c62828"
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Text {
                text: "学号 / 账号"
                font.pixelSize: 14
                color: "#333333"
            }

            AppTextField {
                Layout.fillWidth: true
                label: "请输入学号"
                placeholderText: "请输入学号"
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Text {
                text: "密码"
                font.pixelSize: 14
                color: "#333333"
            }

            AppTextField {
                Layout.fillWidth: true
                label: "请输入密码"
                placeholderText: "请输入密码"
                passwordMode: true
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    text: "验证码"
                    font.pixelSize: 14
                    color: "#333333"
                }

                AppTextField {
                    Layout.fillWidth: true
                    label: "请输入验证码"
                    placeholderText: "请输入验证码"
                }
            }

            ColumnLayout {
                spacing: 4

                Text {
                    text: " "
                    font.pixelSize: 14
                    visible: false
                }

                Rectangle {
                    width: 100
                    height: 40
                    color: "#e0e0e0"
                    radius: 4

                    Text {
                        anchors.centerIn: parent
                        text: "验证码"
                        font.pixelSize: 12
                        color: "#666666"
                    }
                }
            }
        }

        Button {
            text: "刷新验证码"
            flat: true
            Layout.alignment: Qt.AlignRight
            font.pixelSize: 12
            onClicked: {
                console.log("刷新验证码")
            }
        }

        AppButton {
            Layout.fillWidth: true
            text: "登录"
            type: "primary"
            Layout.topMargin: 8
            onClicked: {
                console.log("登录点击")
            }
        }

        Button {
            text: "← 返回"
            flat: true
            Layout.alignment: Qt.AlignHCenter
            onClicked: {
                router.goBack()
            }
        }
    }
}