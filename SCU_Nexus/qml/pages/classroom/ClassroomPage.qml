pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../components"
import "../../components/query"
import "../../styles"

// Context properties are intentionally injected by main.cpp.
// qmllint disable unqualified

Item {
    id: root

    property var selectedRoom: ({})

    Component.onCompleted: classroomViewModel.load()

    function todayIso() {
        const now = new Date()
        const month = String(now.getMonth() + 1).padStart(2, "0")
        const day = String(now.getDate()).padStart(2, "0")
        return now.getFullYear() + "-" + month + "-" + day
    }

    function openRoom(room) {
        root.selectedRoom = room
        detailDialog.room = room
        detailDialog.open()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pagePadding
        spacing: Theme.sectionGap

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacing12

            IconButton {
                visible: classroomViewModel.viewMode !== "campus"
                text: "‹"
                tooltip: classroomViewModel.viewMode === "room" ? "返回教学楼" : "返回校区"
                onClicked: classroomViewModel.goBack()
            }

            ModuleHeader {
                Layout.fillWidth: true
                title: "教室查询"
                subtitle: classroomViewModel.viewMode === "campus"
                    ? "请选择校区"
                    : (classroomViewModel.viewMode === "building"
                       ? classroomViewModel.selectedCampusName + "校区 · 请选择教学楼"
                       : classroomViewModel.selectedCampusName + "校区 · "
                         + classroomViewModel.selectedBuildingName)
            }

            RefreshButton {
                loading: classroomViewModel.loading
                onRefreshRequested: classroomViewModel.refresh()
            }
        }

        SourceAttribution {
            Layout.fillWidth: true
            sourceUrl: "http://zhjw.scu.edu.cn/student/teachingResources/classroomCurriculum/index"
        }

        RowLayout {
            Layout.fillWidth: true
            visible: classroomViewModel.viewMode === "room"
            spacing: Theme.spacing8

            AppButton {
                text: "日期 " + classroomViewModel.selectedDate
                type: "secondary"
                iconText: "📅"
                onClicked: dateDialog.open()
            }

            AppButton {
                visible: !classroomViewModel.selectedDateIsToday
                text: "今天"
                type: "ghost"
                onClicked: classroomViewModel.setSelectedDate(root.todayIso())
            }

            AppButton {
                visible: classroomViewModel.selectedDateIsToday
                text: classroomViewModel.currentPeriod > 0
                    ? "当前第 " + classroomViewModel.currentPeriod + " 节空闲"
                    : "当前无对应节次"
                type: classroomViewModel.filterPeriodStart === classroomViewModel.currentPeriod
                      && classroomViewModel.filterPeriodEnd === classroomViewModel.currentPeriod
                      && classroomViewModel.currentPeriod > 0 ? "primary" : "secondary"
                enabled: classroomViewModel.currentPeriod > 0
                onClicked: classroomViewModel.filterCurrentPeriod()
            }

            AppButton {
                text: classroomViewModel.filterPeriodStart > 0
                    ? "第 " + classroomViewModel.filterPeriodStart + "–"
                      + classroomViewModel.filterPeriodEnd + " 节空闲"
                    : "节次筛选"
                type: "secondary"
                onClicked: periodDialog.open()
            }

            AppButton {
                visible: classroomViewModel.filterPeriodStart > 0
                text: "清除筛选"
                type: "ghost"
                onClicked: classroomViewModel.clearPeriodFilter()
            }

            Item { Layout.fillWidth: true }

            Text {
                text: (classroomViewModel.teachingWeek > 0
                       ? "教学第 " + classroomViewModel.teachingWeek + " 周 · " : "")
                      + classroomViewModel.rooms.length + " 间教室"
                font.pixelSize: Theme.fontLabel
                color: Theme.mutedText
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: campusList
                anchors.fill: parent
                visible: classroomViewModel.viewMode === "campus" && !statePane.visible
                model: classroomViewModel.campuses
                spacing: Theme.spacing8
                clip: true

                delegate: Card {
                    required property int index
                    required property var modelData
                    width: campusList.width
                    implicitHeight: 58
                    elevation: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacing16
                        anchors.rightMargin: Theme.spacing16
                        Text {
                            Layout.fillWidth: true
                            text: modelData.name + "校区"
                            font.pixelSize: Theme.fontSection
                            font.weight: Theme.weightStrong
                            color: Theme.text
                        }
                        Text { text: "›"; font.pixelSize: 24; color: Theme.mutedText }
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: classroomViewModel.selectCampus(index)
                    }
                }
            }

            ListView {
                id: buildingList
                anchors.fill: parent
                visible: classroomViewModel.viewMode === "building"
                         && classroomViewModel.buildings.length > 0
                model: classroomViewModel.buildings
                spacing: Theme.spacing8
                clip: true

                delegate: Card {
                    required property int index
                    required property var modelData
                    width: buildingList.width
                    implicitHeight: 58
                    elevation: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacing16
                        anchors.rightMargin: Theme.spacing16
                        Text {
                            Layout.fillWidth: true
                            text: modelData.name
                            font.pixelSize: Theme.fontSection
                            font.weight: Theme.weightStrong
                            color: Theme.text
                        }
                        Text { text: "›"; font.pixelSize: 24; color: Theme.mutedText }
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: classroomViewModel.selectBuilding(index)
                    }
                }
            }

            EmptyView {
                anchors.fill: parent
                visible: classroomViewModel.viewMode === "building"
                         && classroomViewModel.buildings.length === 0
                         && !classroomViewModel.loading
                title: "暂无教学楼"
                description: "该校区暂未返回教学楼数据"
                actionText: "返回校区"
                onActionTriggered: classroomViewModel.goBack()
            }

            ListView {
                id: roomList
                anchors.fill: parent
                visible: classroomViewModel.viewMode === "room"
                         && !statePane.visible
                         && classroomViewModel.rooms.length > 0
                model: classroomViewModel.rooms
                spacing: Theme.spacing12
                clip: true

                delegate: ClassroomRoomCard {
                    required property var modelData
                    width: roomList.width
                    room: modelData
                    onClicked: function(room) { root.openRoom(room) }
                }
            }

            EmptyView {
                anchors.fill: parent
                visible: classroomViewModel.viewMode === "room"
                         && !statePane.visible
                         && classroomViewModel.rooms.length === 0
                title: classroomViewModel.filterPeriodStart > 0
                    ? "该节次范围暂无空闲教室" : "暂无教室数据"
                description: classroomViewModel.filterPeriodStart > 0
                    ? "可调整或清除节次筛选后重试" : "当前教学楼在所选日期没有返回教室信息"
                actionText: classroomViewModel.filterPeriodStart > 0 ? "清除筛选" : "刷新"
                onActionTriggered: {
                    if (classroomViewModel.filterPeriodStart > 0)
                        classroomViewModel.clearPeriodFilter()
                    else
                        classroomViewModel.refresh()
                }
            }

            QueryStatePane {
                id: statePane
                anchors.fill: parent
                queryState: classroomViewModel.state
                errorMessage: classroomViewModel.errorMessage
                emptyTitle: classroomViewModel.viewMode === "room"
                    ? "暂无教室数据" : "暂无可选数据"
                emptyDescription: "教务系统当前没有返回可用内容"
                loginMessage: "教室查询需要登录教务系统"
                onRetry: classroomViewModel.refresh()
                onLoginRequested: router.navigate("Login")
            }
        }
    }

    ClassroomDateDialog {
        id: dateDialog
        selectedDate: classroomViewModel.selectedDate
        onDateAccepted: function(date) { classroomViewModel.setSelectedDate(date) }
    }

    ClassroomPeriodDialog {
        id: periodDialog
        periodStart: classroomViewModel.filterPeriodStart > 0
            ? classroomViewModel.filterPeriodStart : 1
        periodEnd: classroomViewModel.filterPeriodEnd > 0
            ? classroomViewModel.filterPeriodEnd : 1
        onRangeAccepted: function(periodStart, periodEnd) {
            classroomViewModel.setPeriodFilter(periodStart, periodEnd)
        }
    }

    ClassroomDetailDialog {
        id: detailDialog
        campusName: classroomViewModel.selectedCampusName
        buildingName: classroomViewModel.selectedBuildingName
        queryDate: classroomViewModel.selectedDate
        teachingWeek: classroomViewModel.teachingWeek
    }
}
