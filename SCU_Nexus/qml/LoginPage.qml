import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "components"
import "styles"

Item {
    id: root

    property var auth: appController.authViewModel

    Component.onCompleted: auth.fetchCaptcha()

    Connections {
        target: auth
        // 处理登录成功事件，提示用户并返回上一页。
        function onLoginSucceeded() {
            toastManager.show("登录成功")
            router.goBack()
        }
        // 处理登录失败事件，清空验证码并重新拉取图片。
        function onLoginFailed(message) {
            captchaField.text = ""
            toastManager.show(message, "error")
            auth.fetchCaptcha()
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: Math.min(parent.width - Theme.pagePadding * 2, 460)
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 44
            spacing: 16

            ModuleHeader {
                Layout.fillWidth: true
                title: "登录统一身份认证"
                subtitle: "验证码由用户手动输入；密码加密与真实登录流程由 Person B 服务层接入。"
            }

            Card {
                Layout.fillWidth: true
                implicitHeight: formColumn.implicitHeight + 36

                ColumnLayout {
                    id: formColumn
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 14

                    ErrorView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: auth.errorMessage.length > 0 ? 94 : 0
                        visible: auth.errorMessage.length > 0
                        title: "登录提示"
                        message: auth.errorMessage
                        canRetry: false
                    }

                    AppTextField {
                        id: usernameField
                        Layout.fillWidth: true
                        label: "学号 / 账号"
                        placeholderText: "请输入学号"
                        clearable: true
                        readOnly: auth.loading
                    }

                    AppTextField {
                        id: passwordField
                        Layout.fillWidth: true
                        label: "密码"
                        placeholderText: "请输入密码"
                        passwordMode: true
                        readOnly: auth.loading
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        AppTextField {
                            id: captchaField
                            Layout.fillWidth: true
                            label: "验证码"
                            placeholderText: "请输入验证码"
                            clearable: true
                            readOnly: auth.loading || auth.captchaLoading
                            onAccepted: loginButton.clicked()
                        }

                        Rectangle {
                            Layout.preferredWidth: 132
                            Layout.preferredHeight: 66
                            Layout.alignment: Qt.AlignBottom
                            radius: Theme.smallRadius
                            color: Theme.control
                            border.color: Theme.border

                            Image {
                                anchors.fill: parent
                                anchors.margins: 6
                                source: auth.captchaImageUrl
                                cache: false
                                fillMode: Image.PreserveAspectFit
                                visible: auth.captchaImageUrl.toString().length > 0
                            }

                            LoadingView {
                                anchors.fill: parent
                                compact: true
                                text: ""
                                visible: auth.captchaLoading
                            }

                            Text {
                                anchors.centerIn: parent
                                text: "验证码"
                                font.pixelSize: Theme.fontLabel
                                color: Theme.mutedText
                                visible: !auth.captchaLoading && auth.captchaImageUrl.toString().length === 0
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        AppButton {
                            text: "刷新验证码"
                            type: "secondary"
                            busy: auth.captchaLoading
                            // 清空旧验证码输入并重新请求验证码图片。
                            onClicked: {
                                captchaField.text = ""
                                auth.fetchCaptcha()
                            }
                        }

                        Item { Layout.fillWidth: true }

                        AppButton {
                            id: loginButton
                            text: "登录"
                            busy: auth.loading
                            enabled: !auth.loading
                                     && !auth.captchaLoading
                                     && auth.captchaImageUrl.toString().length > 0
                            onClicked: auth.login(usernameField.text, passwordField.text, captchaField.text)
                        }
                    }
                }
            }

            AppButton {
                Layout.alignment: Qt.AlignHCenter
                text: "返回"
                type: "ghost"
                onClicked: router.goBack()
            }
        }
    }
}
