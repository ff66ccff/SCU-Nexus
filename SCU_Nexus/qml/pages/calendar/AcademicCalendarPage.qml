import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"
import "../../components/query"
import "../../styles"

Item {
    id: root

    Component.onCompleted: academicCalendarViewModel.load()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pagePadding
        spacing: Theme.sectionGap

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ModuleHeader {
                Layout.fillWidth: true
                title: "校历"
                subtitle: "公开校历页面，无需登录"
            }

            ComboBox {
                Layout.preferredWidth: 190
                model: academicCalendarViewModel.entries
                textRole: "title"
                valueRole: "title"
                currentIndex: academicCalendarViewModel.selectedIndex
                enabled: count > 0 && !academicCalendarViewModel.loading
                onActivated: academicCalendarViewModel.selectEntry(index)
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
                anchors.fill: parent
                clip: true
                visible: !statePane.visible

                Column {
                    width: Math.max(parent.width - 16, 320)
                    spacing: 14

                    Repeater {
                        model: academicCalendarViewModel.imageUrls

                        delegate: Rectangle {
                            width: parent.width
                            height: Math.max(220, image.implicitHeight > 0 ? image.implicitHeight * width / Math.max(image.implicitWidth, 1) : 360)
                            radius: Theme.cardRadius
                            color: Theme.surface
                            border.color: Theme.border

                            Image {
                                id: image
                                anchors.fill: parent
                                anchors.margins: 10
                                source: modelData
                                fillMode: Image.PreserveAspectFit
                                asynchronous: true
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    viewer.imageUrl = modelData
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
                emptyDescription: "可以稍后刷新公开校历页面"
                onRetry: academicCalendarViewModel.refresh()
                onLoginRequested: router.navigate("Login")
            }
        }
    }

    CalendarImageViewer { id: viewer }
}
