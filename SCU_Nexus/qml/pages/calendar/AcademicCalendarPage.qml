pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"
import "../../components/query"
import "../../styles"

// Context properties are intentionally injected by main.cpp.
// qmllint disable unqualified

Item {
    id: root

    Component.onCompleted: academicCalendarViewModel.load()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pagePadding
        spacing: Theme.sectionGap

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacing12

            ModuleHeader {
                Layout.fillWidth: true
                title: "校历"
                subtitle: "公开校历页面，无需登录"
            }

            ComboBox {
                Layout.preferredWidth: 210
                model: academicCalendarViewModel.entries
                textRole: "title"
                valueRole: "title"
                currentIndex: academicCalendarViewModel.selectedIndex
                enabled: count > 0 && !academicCalendarViewModel.loading
                onActivated: function(index) {
                    academicCalendarViewModel.selectEntry(index)
                }
            }

            RefreshButton {
                loading: academicCalendarViewModel.loading
                onRefreshRequested: academicCalendarViewModel.refresh()
            }
        }

        LastUpdatedLabel {
            Layout.fillWidth: true
            value: academicCalendarViewModel.lastUpdated
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ScrollView {
                id: calendarScroll
                anchors.fill: parent
                clip: true
                visible: !statePane.visible
                contentWidth: availableWidth

                Column {
                    width: Math.max(calendarScroll.availableWidth, 320)
                    topPadding: Theme.spacing8
                    bottomPadding: Theme.spacing24
                    spacing: Theme.spacing16

                    Repeater {
                        model: academicCalendarViewModel.imageUrls

                        delegate: Card {
                            id: imageCard

                            required property string modelData

                            readonly property int imagePadding: Theme.spacing12
                            readonly property real imageAspectRatio:
                                calendarImage.sourceSize.width > 0
                                ? calendarImage.sourceSize.height
                                  / calendarImage.sourceSize.width
                                : 1.414

                            anchors.horizontalCenter: parent.horizontalCenter
                            width: Math.max(280, Math.min(parent.width - 2 * Theme.spacing16, 980))
                            height: Math.max(
                                        360,
                                        2 * imagePadding
                                        + (width - 2 * imagePadding) * imageAspectRatio)

                            Image {
                                id: calendarImage
                                anchors.fill: parent
                                anchors.margins: imageCard.imagePadding
                                source: imageCard.modelData
                                fillMode: Image.PreserveAspectFit
                                asynchronous: true
                                cache: true
                                visible: status === Image.Ready
                            }

                            BusyIndicator {
                                anchors.centerIn: parent
                                running: calendarImage.status === Image.Loading
                                visible: running
                            }

                            ErrorView {
                                anchors.fill: parent
                                anchors.margins: Theme.spacing16
                                visible: calendarImage.status === Image.Error
                                title: "校历图片加载失败"
                                message: "请检查网络连接后刷新页面"
                                canRetry: false
                            }

                            MouseArea {
                                anchors.fill: parent
                                enabled: calendarImage.status === Image.Ready
                                cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                                onClicked: {
                                    viewer.imageUrl = imageCard.modelData
                                    viewer.open()
                                }
                            }
                        }
                    }
                }
            }

            QueryStatePane {
                id: statePane
                anchors.fill: parent
                queryState: academicCalendarViewModel.state
                errorMessage: academicCalendarViewModel.errorMessage
                emptyTitle: "暂无校历图片"
                emptyDescription: "当前学年尚未发布校历，可以稍后刷新"
                onRetry: academicCalendarViewModel.refresh()
                onLoginRequested: router.navigate("Login")
            }
        }
    }

    CalendarImageViewer {
        id: viewer
        parent: Overlay.overlay
    }
}
