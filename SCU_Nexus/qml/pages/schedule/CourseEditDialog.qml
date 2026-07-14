pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: courseEditDialog
    title: isEditMode ? "编辑课程" : "新增课程"
    width: 480
    height: 560
    modal: true

    property bool isEditMode: false
    property string courseId: ""
    property string courseName: ""
    property string courseTeacher: ""
    property string courseLocation: ""
    property int startWeek: 1
    property int endWeek: 20
    property int dayOfWeek: 1
    property int startSection: 1
    property int endSection: 2
    property int weekType: 0
    property string courseColor: "#42A5F5"
    property int sectionsPerDay: 12
    property string errorText: ""

    signal saved
    signal deleteRequested(string courseId)

    function openForAdd(day, section) {
        isEditMode = false
        courseId = ""
        courseName = ""
        courseTeacher = ""
        courseLocation = ""
        startWeek = 1
        endWeek = 20
        dayOfWeek = day || 1
        startSection = section || 1
        endSection = Math.min(startSection + 1, sectionsPerDay)
        weekType = 0
        errorText = ""
        open()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: "#ffebee"
            visible: courseEditDialog.errorText !== ""
            radius: 4
            Label {
                anchors.centerIn: parent
                text: courseEditDialog.errorText
                color: "#c62828"
                font.pixelSize: 12
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth

            ColumnLayout {
                width: parent.width
                spacing: 10

                // Course Name
                Label { text: "课程名称 *"; font.pixelSize: 12; color: "#666" }
                TextField {
                    Layout.fillWidth: true
                    text: courseEditDialog.courseName
                    onTextChanged: courseEditDialog.courseName = text
                    placeholderText: "请输入课程名称"
                }

                // Teacher
                Label { text: "教师"; font.pixelSize: 12; color: "#666" }
                TextField {
                    Layout.fillWidth: true
                    text: courseEditDialog.courseTeacher
                    onTextChanged: courseEditDialog.courseTeacher = text
                    placeholderText: "请输入教师姓名"
                }

                // Location
                Label { text: "上课地点"; font.pixelSize: 12; color: "#666" }
                TextField {
                    Layout.fillWidth: true
                    text: courseEditDialog.courseLocation
                    onTextChanged: courseEditDialog.courseLocation = text
                    placeholderText: "请输入上课地点"
                }

                RowLayout {
                    spacing: 12
                    ColumnLayout {
                        Label { text: "星期"; font.pixelSize: 12; color: "#666" }
                        ComboBox {
                            Layout.preferredWidth: 140
                            model: ["周一","周二","周三","周四","周五","周六","周日"]
                            currentIndex: courseEditDialog.dayOfWeek - 1
                            onCurrentIndexChanged: courseEditDialog.dayOfWeek = currentIndex + 1
                        }
                    }
                    ColumnLayout {
                        Label { text: "单双周"; font.pixelSize: 12; color: "#666" }
                        ComboBox {
                            Layout.preferredWidth: 140
                            model: ["每周", "单周", "双周"]
                            currentIndex: courseEditDialog.weekType
                            onCurrentIndexChanged: courseEditDialog.weekType = currentIndex
                        }
                    }
                }

                RowLayout {
                    spacing: 12
                    ColumnLayout {
                        Label { text: "起始周"; font.pixelSize: 12; color: "#666" }
                        SpinBox {
                            Layout.preferredWidth: 100
                            from: 1; to: 30
                            value: courseEditDialog.startWeek
                            onValueChanged: courseEditDialog.startWeek = value
                        }
                    }
                    ColumnLayout {
                        Label { text: "结束周"; font.pixelSize: 12; color: "#666" }
                        SpinBox {
                            Layout.preferredWidth: 100
                            from: 1; to: 30
                            value: courseEditDialog.endWeek
                            onValueChanged: courseEditDialog.endWeek = value
                        }
                    }
                }

                RowLayout {
                    spacing: 12
                    ColumnLayout {
                        Label { text: "起始节次"; font.pixelSize: 12; color: "#666" }
                        SpinBox {
                            Layout.preferredWidth: 100
                            from: 1; to: courseEditDialog.sectionsPerDay
                            value: courseEditDialog.startSection
                            onValueChanged: courseEditDialog.startSection = value
                        }
                    }
                    ColumnLayout {
                        Label { text: "结束节次"; font.pixelSize: 12; color: "#666" }
                        SpinBox {
                            Layout.preferredWidth: 100
                            from: 1; to: courseEditDialog.sectionsPerDay
                            value: courseEditDialog.endSection
                            onValueChanged: courseEditDialog.endSection = value
                        }
                    }
                }

                // Color picker
                Label { text: "课程颜色"; font.pixelSize: 12; color: "#666" }
                RowLayout {
                    spacing: 6
                    Repeater {
                        model: ["#EF5350","#EC407A","#AB47BC","#7E57C2","#5C6BC0","#42A5F5","#26C6DA","#26A69A","#66BB6A","#9CCC65","#FFA726","#8D6E63"]
                        Rectangle {
                            id: colorSwatch

                            required property string modelData
                            width: 28; height: 28; radius: 14
                            color: colorSwatch.modelData
                            border.width: courseEditDialog.courseColor
                                          === colorSwatch.modelData ? 3 : 0
                            border.color: "#333"
                            MouseArea {
                                anchors.fill: parent
                                onClicked: courseEditDialog.courseColor
                                           = colorSwatch.modelData
                            }
                        }
                    }
                }
            }
        }

        // Bottom buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Item { Layout.fillWidth: true }
            Button {
                text: "删除"
                visible: courseEditDialog.isEditMode
                onClicked: courseEditDialog.deleteRequested(courseEditDialog.courseId)
            }
            Button {
                text: "取消"
                onClicked: courseEditDialog.close()
            }
            Button {
                text: "保存"
                highlighted: true
                onClicked: {
                    if (courseEditDialog.courseName.trim() === "") {
                        courseEditDialog.errorText = "课程名不能为空"
                        return
                    }
                    courseEditDialog.errorText = ""
                    courseEditDialog.saved()
                }
            }
        }
    }
}
