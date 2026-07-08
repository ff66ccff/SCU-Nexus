import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    color: "#f5f5f5"

    property var navItems: [
        { name: "课表", route: "Schedule", icon: "📅" },
        { name: "校历", route: "AcademicCalendar", icon: "📆" },
        { name: "考表", route: "ExamPlan", icon: "📝" },
        { name: "成绩", route: "Grades", icon: "📊" },
        { name: "设置", route: "Settings", icon: "⚙️" }
    ]

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // 左侧导航栏
        Rectangle {
            id: navPanel
            width: 180
            Layout.fillHeight: true
            color: "#ffffff"
            border {
                color: "#e0e0e0"
                width: 1
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 4

                Rectangle {
                    Layout.fillWidth: true
                    height: 50
                    color: "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "SCU Nexus"
                        font.pixelSize: 18
                        font.bold: true
                        color: "#1976d2"
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#e0e0e0"
                    Layout.bottomMargin: 8
                }

                Repeater {
                    model: navItems
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        height: 40
                        radius: 6
                        color: {
                            if (router.currentRoute === modelData.route)
                                return "#e3f2fd"
                            return mouseArea.containsMouse ? "#f5f5f5" : "transparent"
                        }
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            spacing: 10
                            Text {
                                text: modelData.icon
                                font.pixelSize: 18
                            }
                            Text {
                                text: modelData.name
                                font.pixelSize: 14
                                color: router.currentRoute === modelData.route ? "#1976d2" : "#333333"
                                font.weight: router.currentRoute === modelData.route ? Font.Bold : Font.Normal
                            }
                        }
                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                router.navigate(modelData.route)
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                Text {
                    text: "v0.1.0"
                    font.pixelSize: 11
                    color: "#999999"
                    Layout.alignment: Qt.AlignHCenter
                    Layout.bottomMargin: 8
                }
            }
        }
        // 右侧内容区
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#f5f5f5"

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    height: 50
                    color: "#ffffff"
                    border {
                        color: "#e0e0e0"
                        width: 1
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 20
                        anchors.rightMargin: 20

                        Text {
                            text: getRouteTitle(router.currentRoute)
                            font.pixelSize: 18
                            font.bold: true
                            color: "#333333"
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            spacing: 8
                            Rectangle {
                                width: 8
                                height: 8
                                radius: 4
                                color: appController.loggedIn ? "#4CAF50" : "#bdbdbd"
                            }
                            Text {
                                text: appController.loggedIn ? "已登录" : "未登录"
                                font.pixelSize: 12
                                color: "#666666"
                            }
                        }
                    }
                }

                Loader {
                    id: pageLoader
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    source: getPageSource(router.currentRoute)
                }
            }
        }
    }
    function getRouteTitle(route) {
        var titles = {
            "Schedule": "课表管理",
            "AcademicCalendar": "校历查询",
            "ExamPlan": "考表查询",
            "Grades": "教务成绩",
            "Settings": "设置",
            "Login": "登录"
        }
        return titles[route] || "SCU Nexus"
    }

    function getPageSource(route) {
        var sources = {
            "Schedule": "qrc:/qml/pages/SchedulePagePlaceholder.qml",
            "AcademicCalendar": "qrc:/qml/pages/AcademicCalendarPagePlaceholder.qml",
            "ExamPlan": "qrc:/qml/pages/ExamPlanPagePlaceholder.qml",
            "Grades": "qrc:/qml/pages/GradesPagePlaceholder.qml",
            "Settings": "qrc:/qml/SettingsPage.qml",
            "Login": "qrc:/qml/LoginPage.qml"
        }
        return sources[route] || ""
    }
}