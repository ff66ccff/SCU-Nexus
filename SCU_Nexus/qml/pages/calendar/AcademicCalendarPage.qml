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

    property bool aiEnabled: appSettings.hasQwenApiKey
    property bool aiReady: false
    property bool aiThinking: false
    readonly property bool structuredAvailable:
        academicCalendarViewModel.hasStructuredCalendar

    function newThinkingInterval() {
        return 20000 + Math.floor(Math.random() * 40001)
    }

    function finishThinking() {
        aiTimer.stop()
        root.aiThinking = false
        root.aiReady = true
    }

    function openOriginalImage(imageUrl) {
        viewer.imageUrl = imageUrl
        viewer.open()
    }

    Component.onCompleted: {
        academicCalendarViewModel.load()
        if (root.aiEnabled) {
            root.aiThinking = true
            aiTimer.interval = root.newThinkingInterval()
            aiTimer.start()
        }
    }

    Timer {
        id: aiTimer
        objectName: "academicCalendarAiTimer"
        repeat: false
        onTriggered: root.finishThinking()
    }

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
                objectName: "academicCalendarYearSelector"
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
                objectName: "academicCalendarRefreshButton"
                loading: academicCalendarViewModel.loading
                onRefreshRequested: academicCalendarViewModel.refresh()
            }
        }

        SourceAttribution {
            objectName: "academicCalendarSourceAttribution"
            Layout.fillWidth: true
            sourceUrl: "https://jwc.scu.edu.cn/cdxl.htm"
        }

        LastUpdatedLabel {
            objectName: "academicCalendarLastUpdated"
            Layout.fillWidth: true
            value: academicCalendarViewModel.lastUpdated
        }

        Item {
            id: contentArea
            Layout.fillWidth: true
            Layout.fillHeight: true

            Item {
                id: thinkingState
                objectName: "academicCalendarThinkingState"
                anchors.fill: parent
                visible: root.aiThinking

                Column {
                    anchors.centerIn: parent
                    spacing: Theme.spacing12

                    BusyIndicator {
                        anchors.horizontalCenter: parent.horizontalCenter
                        running: thinkingState.visible
                        visible: running
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "thinking..."
                        font.pixelSize: Theme.fontBody
                        color: Theme.mutedText
                    }
                }
            }

            StructuredAcademicCalendarView {
                id: structuredView
                objectName: "academicCalendarStructuredState"
                anchors.fill: parent
                visible: root.aiReady && root.structuredAvailable
                calendarData: academicCalendarViewModel.structuredCalendar
                onViewOriginalRequested: function(imageUrl) {
                    root.openOriginalImage(imageUrl)
                }
            }

            Item {
                id: imageFallback
                objectName: "academicCalendarImageFallback"
                anchors.fill: parent
                visible: !root.aiThinking
                         && !(root.aiReady && root.structuredAvailable)

                ScrollView {
                    id: calendarScroll
                    objectName: "academicCalendarImageScroll"
                    anchors.fill: parent
                    clip: true
                    visible: !statePane.visible
                    contentWidth: availableWidth

                    Column {
                        width: Math.max(calendarScroll.availableWidth, 320)
                        topPadding: Theme.spacing8
                        bottomPadding: Theme.spacing24
                        spacing: Theme.spacing16

                        Text {
                            objectName: "academicCalendarStructuredFallbackMessage"
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: Math.max(280, Math.min(parent.width - 2 * Theme.spacing16, 980))
                            visible: root.aiReady && !root.structuredAvailable
                            text: "当前学年暂未完成智能解析"
                            font.pixelSize: Theme.fontBody
                            color: Theme.mutedText
                            wrapMode: Text.WordWrap
                        }

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
                                        root.openOriginalImage(imageCard.modelData)
                                    }
                                }
                            }
                        }
                    }
                }

                QueryStatePane {
                    id: statePane
                    objectName: "academicCalendarQueryStatePane"
                    anchors.fill: parent
                    queryState: academicCalendarViewModel.state
                    errorMessage: academicCalendarViewModel.errorMessage
                    emptyTitle: "暂无校历图片"
                    emptyDescription: "当前学年尚未发布校历，可以稍后刷新"
                    onRetry: academicCalendarViewModel.refresh()
                    onLoginRequested: router.navigate("Login")
                }
            }

            Card {
                objectName: "academicCalendarEnableAiPrompt"
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: Theme.spacing16
                width: enableAiButton.implicitWidth + Theme.spacing16
                height: enableAiButton.implicitHeight + Theme.spacing16
                visible: !root.aiEnabled
                elevation: 2

                AppButton {
                    id: enableAiButton
                    objectName: "academicCalendarEnableAiButton"
                    anchors.centerIn: parent
                    text: "开启智能校历解析"
                    type: "secondary"
                    onClicked: router.navigate("Settings")
                }
            }
        }
    }

    CalendarImageViewer {
        id: viewer
        objectName: "academicCalendarImageViewer"
        parent: Overlay.overlay
    }
}
