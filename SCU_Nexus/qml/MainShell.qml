import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "components"
import "styles"

Rectangle {
    id: root

    color: Theme.background

    property var navItems: [
        { name: "课表", route: "Schedule", icon: "课" },
        { name: "校历", route: "AcademicCalendar", icon: "历" },
        { name: "考表", route: "ExamPlan", icon: "考" },
        { name: "成绩", route: "Grades", icon: "绩" },
        { name: "设置", route: "Settings", icon: "设" }
    ]

    Connections {
        target: appController
        function onSessionExpired(message) {
            toastManager.show(message, "error")
            router.navigate("Login")
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.preferredWidth: Theme.navWidth
            Layout.fillHeight: true
            color: Theme.surface
            border.color: Theme.border

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 8

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Text {
                        text: "SCU Nexus"
                        font.pixelSize: 18
                        font.weight: Theme.weightStrong
                        color: Theme.text
                    }

                    Text {
                        text: "校园学习工作台"
                        font.pixelSize: Theme.fontCaption
                        color: Theme.mutedText
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Theme.border
                    Layout.topMargin: 6
                    Layout.bottomMargin: 4
                }

                Repeater {
                    model: navItems
                    delegate: Rectangle {
                        id: navItem
                        Layout.fillWidth: true
                        height: 40
                        radius: Theme.smallRadius
                        readonly property bool active: router.currentRoute === modelData.route
                        color: navItem.active
                               ? Theme.primarySoft
                               : (navMouse.containsMouse ? Theme.control : "transparent")

                        // Active indicator: a left accent bar encodes the current page.
                        Rectangle {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            width: 3
                            height: parent.height - 12
                            radius: 1.5
                            color: Theme.primary
                            visible: navItem.active
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 8
                            spacing: 10

                            Rectangle {
                                width: 24
                                height: 24
                                radius: 12
                                color: navItem.active ? Theme.primary : Theme.control

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.icon
                                    font.pixelSize: Theme.fontCaption
                                    font.weight: Theme.weightStrong
                                    color: navItem.active ? "white" : Theme.mutedText
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: modelData.name
                                font.pixelSize: Theme.fontBody
                                font.weight: navItem.active ? Theme.weightStrong : Theme.weightNormal
                                color: navItem.active ? Theme.primary : Theme.text
                                elide: Text.ElideRight
                            }
                        }

                        MouseArea {
                            id: navMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: router.navigate(modelData.route)
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 58
                    radius: Theme.cardRadius
                    color: Theme.surfaceMuted
                    border.color: Theme.border

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 8

                        Rectangle {
                            width: 9
                            height: 9
                            radius: 4.5
                            color: appController.loggedIn ? Theme.success : Theme.placeholder
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1

                            Text {
                                text: appController.loggedIn ? "已登录" : "未登录"
                                font.pixelSize: Theme.fontLabel
                                color: Theme.text
                            }

                            Text {
                                text: "v" + appController.appVersion
                                font.pixelSize: Theme.fontMicro
                                color: Theme.mutedText
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.background

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    height: Theme.topBarHeight
                    color: Theme.surface
                    border.color: Theme.border

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.pagePadding
                        anchors.rightMargin: Theme.pagePadding
                        spacing: 10

                        IconButton {
                            visible: router.canGoBack
                            text: "‹"
                            tooltip: "返回"
                            onClicked: router.goBack()
                        }

                        Text {
                            Layout.fillWidth: true
                            text: router.routeTitle
                            font.pixelSize: Theme.fontTitle
                            font.weight: Theme.weightStrong
                            color: Theme.text
                            elide: Text.ElideRight
                        }

                        IconButton {
                            text: "↻"
                            tooltip: "刷新当前页面"
                            onClicked: toastManager.show("刷新入口已预留，等待业务 ViewModel 接入。")
                        }

                        AppButton {
                            text: appController.loggedIn ? "账户" : "登录"
                            type: "secondary"
                            onClicked: router.navigate(appController.loggedIn ? "Settings" : "Login")
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Loader {
                        id: pageLoader
                        anchors.fill: parent
                        source: protectedRouteBlocked(router.currentRoute)
                                ? ""
                                : pageSource(router.currentRoute)
                    }

                    LoginRequiredView {
                        anchors.fill: parent
                        visible: protectedRouteBlocked(router.currentRoute)
                        message: router.currentRoute === "ExamPlan"
                                 ? "考表查询需要登录教务系统"
                                 : "教务成绩需要登录教务系统"
                        onLoginRequested: router.navigate("Login")
                    }
                }
            }
        }
    }

    ToastHost { }

    function protectedRouteBlocked(route) {
        return !appController.loggedIn && (route === "ExamPlan" || route === "Grades")
    }

    function pageSource(route) {
        if (route === "Schedule") return "qrc:/SCU_Nexus/qml/pages/SchedulePagePlaceholder.qml"
        if (route === "AcademicCalendar") return "qrc:/SCU_Nexus/qml/pages/AcademicCalendarPagePlaceholder.qml"
        if (route === "ExamPlan") return "qrc:/SCU_Nexus/qml/pages/ExamPlanPagePlaceholder.qml"
        if (route === "Grades") return "qrc:/SCU_Nexus/qml/pages/GradesPagePlaceholder.qml"
        if (route === "Settings") return "qrc:/SCU_Nexus/qml/SettingsPage.qml"
        if (route === "Login") return "qrc:/SCU_Nexus/qml/LoginPage.qml"
        return "qrc:/SCU_Nexus/qml/pages/SchedulePagePlaceholder.qml"
    }
}
