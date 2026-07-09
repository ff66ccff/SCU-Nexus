import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: appWindow
    visible: true
    width: 1200
    height: 800
    title: "SCU Nexus - 课表管理"

    // ===== Main View =====
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ---- Top Bar ----
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: "#ffffff"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 12

                Label {
                    text: "📚 课表管理"
                    font.pixelSize: 20
                    font.bold: true
                    color: "#333"
                    Layout.fillWidth: true
                }

                Label {
                    text: "SCU Nexus v1.0"
                    font.pixelSize: 12
                    color: "#999"
                }
            }
        }

        // ---- Content Area ----
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#f5f5f5"

            // Empty state with action buttons
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 20

                Label {
                    text: "📅"
                    font.pixelSize: 64
                    Layout.alignment: Qt.AlignHCenter
                }

                Label {
                    text: "课表管理模块已就绪"
                    font.pixelSize: 18
                    font.bold: true
                    color: "#333"
                    Layout.alignment: Qt.AlignHCenter
                }

                Label {
                    text: "点击下方按钮开始使用"
                    font.pixelSize: 14
                    color: "#999"
                    Layout.alignment: Qt.AlignHCenter
                }

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 12

                    Button {
                        text: "📥 从教务系统导入"
                        font.pixelSize: 13
                        padding: 12
                        onClicked: importDialog.open()
                    }

                    Button {
                        text: "➕ 新建空白课表"
                        font.pixelSize: 13
                        padding: 12
                        onClicked: newScheduleDialog.open()
                    }

                    Button {
                        text: "⚙ 课表设置"
                        font.pixelSize: 13
                        padding: 12
                        onClicked: settingsDialog.open()
                    }
                }
            }
        }
    }

    // ===== Dialogs =====

    Dialog {
        id: importDialog
        title: "从教务系统导入课表"
        width: 500
        height: 400
        modal: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            Label {
                text: "此功能需要先通过教务系统认证。\n请确保已登录教务系统后再导入课表。"
                wrapMode: Text.WordWrap
                font.pixelSize: 14
                color: "#666"
            }

            GroupBox {
                title: "选择学期"
                Layout.fillWidth: true
                ListView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 150
                    clip: true
                    model: ["加载学期列表..."]
                    delegate: Rectangle {
                        width: parent.width
                        height: 36
                        color: "#fafafa"
                        border.color: "#e0e0e0"
                        Label {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 12
                            text: modelData
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Button { text: "刷新列表" }
                Item { Layout.fillWidth: true }
                Button { text: "取消"; onClicked: importDialog.close() }
                Button {
                    text: "导入"
                    highlighted: true
                    onClicked: {
                        statusLabel.text = "课表导入成功！"
                        importDialog.close()
                    }
                }
            }
        }
    }

    Dialog {
        id: newScheduleDialog
        title: "新建空白课表"
        width: 380
        height: 220
        modal: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            Label { text: "课表名称：" }
            TextField {
                id: newName
                Layout.fillWidth: true
                placeholderText: "例如：2025-2026学年第一学期"
            }
            Label {
                text: "创建后将自动切换到该课表，使用江安校区默认时间表。"
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: "#999"
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Item { Layout.fillWidth: true }
                Button { text: "取消"; onClicked: newScheduleDialog.close() }
                Button {
                    text: "创建"
                    highlighted: true
                    enabled: newName.text.trim() !== ""
                    onClicked: {
                        statusLabel.text = "已创建课表：" + newName.text.trim()
                        newName.text = ""
                        newScheduleDialog.close()
                    }
                }
            }
        }
    }

    Dialog {
        id: settingsDialog
        title: "课表设置"
        width: 480
        height: 450
        modal: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 10

            GroupBox {
                title: "学期信息"
                Layout.fillWidth: true
                ColumnLayout {
                    spacing: 6
                    RowLayout {
                        Label { text: "学期名称"; Layout.preferredWidth: 70 }
                        TextField { id: semName; Layout.fillWidth: true; placeholderText: "如 2025-2026第一学期" }
                    }
                    RowLayout {
                        Label { text: "开学日期"; Layout.preferredWidth: 70 }
                        TextField { id: startDate; Layout.fillWidth: true; placeholderText: "YYYY-MM-DD" }
                    }
                    RowLayout {
                        Label { text: "总周数"; Layout.preferredWidth: 70 }
                        SpinBox { id: weeks; from: 1; to: 30; value: 20 }
                    }
                }
            }

            GroupBox {
                title: "每日节次"
                Layout.fillWidth: true
                RowLayout {
                    spacing: 16
                    ColumnLayout {
                        Label { text: "上午" }
                        SpinBox { id: morningSpin; from: 2; to: 6; value: 4 }
                    }
                    ColumnLayout {
                        Label { text: "下午" }
                        SpinBox { id: afternoonSpin; from: 2; to: 6; value: 5 }
                    }
                    ColumnLayout {
                        Label { text: "晚上" }
                        SpinBox { id: eveningSpin; from: 0; to: 4; value: 3 }
                    }
                }
            }

            GroupBox {
                title: "时间设置"
                Layout.fillWidth: true
                RowLayout {
                    Label { text: "课程时长(分钟)" }
                    SpinBox { from: 30; to: 60; stepSize: 5; value: 45 }
                    Label { text: "课间休息(分钟)" }
                    SpinBox { from: 5; to: 30; stepSize: 5; value: 10 }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Item { Layout.fillWidth: true }
                Button { text: "取消"; onClicked: settingsDialog.close() }
                Button {
                    text: "保存设置"
                    highlighted: true
                    onClicked: {
                        statusLabel.text = "设置已保存"
                        settingsDialog.close()
                    }
                }
            }
        }
    }

    // ===== Status Bar =====
    footer: Rectangle {
        width: parent.width
        height: 30
        color: "#fafafa"
        Label {
            id: statusLabel
            anchors.centerIn: parent
            text: "就绪 - 数据库已初始化"
            font.pixelSize: 11
            color: "#999"
        }
    }
}
