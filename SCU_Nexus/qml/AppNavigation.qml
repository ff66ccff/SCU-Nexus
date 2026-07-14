pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "components"
import "styles"

// Context properties (router/appController) are intentionally injected by main.cpp.
// qmllint disable unqualified

Item {
    id: root

    property bool compact: false

    readonly property var navItems: [
        { name: "课表", route: "Schedule", icon: "\uE787" },
        { name: "校历", route: "AcademicCalendar", icon: "\uE823" },
        { name: "考表", route: "ExamPlan", icon: "\uE9D9" },
        { name: "成绩", route: "Grades", icon: "\uE82D" },
        { name: "设置", route: "Settings", icon: "\uE713" }
    ]

    implicitWidth: compact ? Theme.navCompactWidth : Theme.navWidth

    Rectangle {
        anchors.fill: parent
        color: Theme.navBackground

        Rectangle {
            anchors.right: parent.right
            width: 1
            height: parent.height
            color: Theme.border
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacing12
        spacing: Theme.spacing4

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            Layout.leftMargin: root.compact ? 0 : Theme.spacing4
            spacing: Theme.spacing12

            Rectangle {
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                radius: Theme.cardRadius
                color: Theme.accent

                Text {
                    anchors.centerIn: parent
                    text: "川"
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSection
                    font.weight: Theme.weightStrong
                    color: Theme.accentText
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                visible: !root.compact
                spacing: 0

                Text {
                    Layout.fillWidth: true
                    text: "SCU Nexus"
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSection
                    font.weight: Theme.weightStrong
                    color: Theme.text
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    text: "校园学习工作台"
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontMicro
                    color: Theme.mutedText
                    elide: Text.ElideRight
                }
            }
        }

        Item { Layout.preferredHeight: Theme.spacing8 }

        Repeater {
            model: root.navItems

            delegate: Rectangle {
                id: navItem

                required property var modelData

                Layout.fillWidth: true
                Layout.preferredHeight: 42
                radius: Theme.smallRadius

                readonly property bool active: router.currentRoute === modelData.route
                color: active
                       ? Theme.subtleActive
                       : (navMouse.containsMouse ? Theme.subtleHover : "transparent")

                Rectangle {
                    anchors.left: parent.left
                    anchors.leftMargin: 2
                    anchors.verticalCenter: parent.verticalCenter
                    width: 3
                    height: 18
                    radius: 2
                    color: Theme.accent
                    visible: navItem.active
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: root.compact ? 0 : Theme.spacing12
                    anchors.rightMargin: root.compact ? 0 : Theme.spacing8
                    spacing: Theme.spacing12

                    Text {
                        Layout.preferredWidth: root.compact ? parent.width : 24
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        text: navItem.modelData.icon
                        font.family: Theme.iconFontFamily
                        font.pixelSize: 18
                        color: navItem.active ? Theme.accent : Theme.mutedText
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: !root.compact
                        text: navItem.modelData.name
                        font.family: Theme.fontFamily
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
                    onClicked: router.navigate(navItem.modelData.route)
                }

                ToolTip.visible: root.compact && navMouse.containsMouse
                ToolTip.text: navItem.modelData.name
                ToolTip.delay: 500
            }
        }

        Item { Layout.fillHeight: true }

        Card {
            Layout.fillWidth: true
            implicitHeight: root.compact ? 44 : 60
            elevation: 0

            RowLayout {
                anchors.fill: parent
                anchors.margins: root.compact ? Theme.spacing8 : Theme.spacing12
                spacing: Theme.spacing8

                Rectangle {
                    Layout.preferredWidth: 10
                    Layout.preferredHeight: 10
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    radius: 5
                    color: appController.loggedIn ? Theme.success : Theme.placeholder
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    visible: !root.compact
                    spacing: 1

                    Text {
                        text: appController.loggedIn ? "已登录" : "未登录"
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontLabel
                        color: Theme.text
                    }

                    Text {
                        text: "v" + appController.appVersion
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontMicro
                        color: Theme.mutedText
                    }
                }
            }
        }
    }
}
