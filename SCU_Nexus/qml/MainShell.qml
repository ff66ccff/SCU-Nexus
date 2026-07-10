import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "components"
import "styles"

// 应用主壳：仿 FluentWinUI3 NavigationView 布局。
// 左侧 Mica 导航栏（圆角选中项 + 品牌红指示条），顶部 Mica 标题栏，中央为业务页 Loader。
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
        // 处理会话过期事件，提示用户并跳转到登录页。
        function onSessionExpired(message) {
            toastManager.show(message, "error")
            router.navigate("Login")
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ---------- 左侧导航 Pane ----------
        Rectangle {
            Layout.preferredWidth: Theme.navWidth
            Layout.fillHeight: true
            color: Theme.navBackground

            // 与内容区之间的细描边。
            Rectangle {
                anchors.right: parent.right
                width: 1
                height: parent.height
                color: Theme.border
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 6

                // 品牌区
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 6
                    Layout.topMargin: 4
                    spacing: 10

                    Rectangle {
                        width: 30
                        height: 30
                        radius: 8
                        color: Theme.accent

                        Text {
                            anchors.centerIn: parent
                            text: "川"
                            font.pixelSize: 16
                            font.weight: Theme.weightStrong
                            color: Theme.accentText
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        Text {
                            text: "SCU Nexus"
                            font.pixelSize: 16
                            font.weight: Theme.weightStrong
                            color: Theme.text
                        }

                        Text {
                            text: "校园学习工作台"
                            font.pixelSize: Theme.fontMicro
                            color: Theme.mutedText
                        }
                    }
                }

                Item { Layout.preferredHeight: 6 }

                // 导航项
                Repeater {
                    model: navItems
                    delegate: Rectangle {
                        id: navItem
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        radius: Theme.smallRadius
                        readonly property bool active: router.currentRoute === modelData.route
                        color: navItem.active
                               ? Theme.subtleActive
                               : (navMouse.containsMouse ? Theme.subtleHover : "transparent")

                        // Fluent 选中指示：左侧圆角品牌红竖条。
                        Rectangle {
                            anchors.left: parent.left
                            anchors.leftMargin: 3
                            anchors.verticalCenter: parent.verticalCenter
                            width: 3
                            height: 16
                            radius: 1.5
                            color: Theme.accent
                            visible: navItem.active
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 10
                            spacing: 12

                            Text {
                                Layout.preferredWidth: 20
                                horizontalAlignment: Text.AlignHCenter
                                text: modelData.icon
                                font.pixelSize: Theme.fontBody
                                font.weight: Theme.weightStrong
                                color: navItem.active ? Theme.accent : Theme.mutedText
                            }

                            Text {
                                Layout.fillWidth: true
                                text: modelData.name
                                font.pixelSize: Theme.fontBody
                                font.weight: navItem.active ? Theme.weightStrong : Theme.weightNormal
                                color: Theme.text
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

                // 登录状态卡片
                Card {
                    Layout.fillWidth: true
                    implicitHeight: 56

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

        // ---------- 右侧内容 ----------
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.background

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // 顶部标题栏（Mica，底部细分割线）
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Theme.topBarHeight
                    color: Theme.background

                    Rectangle {
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: 1
                        color: Theme.border
                    }

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
                            onClicked: {
                                if (router.currentRoute === "Schedule") {
                                    scheduleViewModel.load()
                                } else if (router.currentRoute === "AcademicCalendar") {
                                    academicCalendarViewModel.refresh()
                                } else if (router.currentRoute === "ExamPlan") {
                                    examPlanViewModel.refresh()
                                } else if (router.currentRoute === "Grades") {
                                    gradesViewModel.refreshSchemeScores()
                                    gradesViewModel.refreshPassingScores()
                                }
                            }
                        }

                        AppButton {
                            text: appController.loggedIn ? "账户" : "登录"
                            type: appController.loggedIn ? "secondary" : "primary"
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

    // 判断受保护页面是否因未登录而需要拦截。
    function protectedRouteBlocked(route) {
        return !appController.loggedIn && (route === "ExamPlan" || route === "Grades")
    }

    // 根据当前路由返回对应的 QML 页面资源路径。
    function pageSource(route) {
        if (route === "Schedule") return "qrc:/SCU_Nexus/qml/pages/schedule/SchedulePage.qml"
        if (route === "AcademicCalendar") return "qrc:/SCU_Nexus/qml/pages/calendar/AcademicCalendarPage.qml"
        if (route === "ExamPlan") return "qrc:/SCU_Nexus/qml/pages/exam/ExamPlanPage.qml"
        if (route === "Grades") return "qrc:/SCU_Nexus/qml/pages/grades/GradesPage.qml"
        if (route === "Settings") return "qrc:/SCU_Nexus/qml/SettingsPage.qml"
        if (route === "Login") return "qrc:/SCU_Nexus/qml/LoginPage.qml"
        return "qrc:/SCU_Nexus/qml/pages/schedule/SchedulePage.qml"
    }
}
