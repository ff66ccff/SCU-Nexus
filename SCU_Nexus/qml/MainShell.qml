import QtQuick 2.15
import QtQuick.Layouts 1.15
import "components"
import "styles"

// Context properties are intentionally injected by main.cpp.
// qmllint disable unqualified

// 应用主壳：仿 FluentWinUI3 NavigationView 布局。
// 左侧 Mica 导航栏（圆角选中项 + 品牌红指示条），顶部 Mica 标题栏，中央为业务页 Loader。
Rectangle {
    id: root

    color: Theme.background

    readonly property bool compactNavigation: width < 1040

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
        AppNavigation {
            Layout.preferredWidth: implicitWidth
            Layout.fillHeight: true
            compact: root.compactNavigation
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
