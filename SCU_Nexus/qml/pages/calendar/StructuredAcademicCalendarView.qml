pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"
import "../../styles"

Item {
    id: root

    property var calendarData: ({})
    property int selectedTermIndex: 0
    readonly property var terms: calendarData && calendarData.terms
                                 ? calendarData.terms : []
    readonly property var selectedTerm: root.selectedTermIndex >= 0
                                        && root.terms.length > root.selectedTermIndex
                                        ? root.terms[root.selectedTermIndex] : ({})
    readonly property bool compact: width < 620
    signal viewOriginalRequested(string imageUrl)

    onCalendarDataChanged: root.selectedTermIndex = 0

    function safeList(value) {
        if (value === undefined || value === null || typeof value === "string")
            return []
        return typeof value.length === "number" && value.length >= 0 ? value : []
    }

    function stringValue(value) {
        return value === undefined || value === null ? "" : String(value)
    }

    function formatDate(value) {
        if (value === undefined || value === null || value === "")
            return ""

        if (typeof value === "string") {
            const match = value.match(/^(\d{4})-(\d{1,2})-(\d{1,2})/)
            if (match)
                return match[1] + "年" + Number(match[2]) + "月" + Number(match[3]) + "日"
        }

        try {
            const formatted = Qt.formatDate(value, "yyyy年M月d日")
            return formatted && formatted !== "Invalid Date" ? formatted : root.stringValue(value)
        } catch (error) {
            return root.stringValue(value)
        }
    }

    function dateRange(startDate, endDate) {
        const start = root.formatDate(startDate)
        const end = root.formatDate(endDate)
        if (!start)
            return end
        if (!end || start === end)
            return start
        return start + " – " + end
    }

    function phaseLabel(phase) {
        switch (root.stringValue(phase)) {
        case "teaching": return "教学周"
        case "exam": return "考试周"
        case "practice": return "实践周"
        case "winter-break":
        case "winter_break": return "寒假"
        case "summer-break":
        case "summer_break": return "暑假"
        default: return "其他安排"
        }
    }

    function phaseBackground(phase) {
        switch (root.stringValue(phase)) {
        case "teaching": return Theme.primarySoft
        case "exam": return Theme.dangerSoft
        case "practice": return Theme.surfaceMuted
        case "winter-break":
        case "winter_break": return Theme.controlPressed
        case "summer-break":
        case "summer_break": return Theme.subtleActive
        default: return Theme.control
        }
    }

    function phaseForeground(phase) {
        switch (root.stringValue(phase)) {
        case "teaching": return Theme.accent
        case "exam": return Theme.danger
        case "practice": return Theme.success
        case "summer-break":
        case "summer_break": return Theme.accent
        default: return Theme.mutedText
        }
    }

    function eventTypeLabel(type) {
        switch (root.stringValue(type)) {
        case "registration": return "报到注册"
        case "make-up-exam": return "补缓考"
        case "ceremony": return "典礼"
        case "instruction": return "教学安排"
        case "holiday": return "节假日"
        case "athletics": return "体育活动"
        case "final-exam": return "期末考试"
        case "vacation": return "假期"
        default: return "校历事项"
        }
    }

    function dateSortKey(value) {
        if (value instanceof Date && !isNaN(value.getTime()))
            return value.getTime()
        const text = root.stringValue(value)
        const match = text.match(/^(\d{4})-(\d{1,2})-(\d{1,2})/)
        if (!match)
            return Number.MAX_SAFE_INTEGER
        return Date.UTC(Number(match[1]), Number(match[2]) - 1, Number(match[3]))
    }

    function sortedEvents(events) {
        const source = root.safeList(events)
        const decorated = []
        for (let index = 0; index < source.length; ++index)
            decorated.push({ "item": source[index], "index": index })
        decorated.sort(function(left, right) {
            const difference = root.dateSortKey(left.item ? left.item.startDate : undefined)
                             - root.dateSortKey(right.item ? right.item.startDate : undefined)
            return difference === 0 ? left.index - right.index : difference
        })
        const result = []
        for (let index = 0; index < decorated.length; ++index)
            result.push(decorated[index].item)
        return result
    }

    function sortedNotes(notes) {
        const source = root.safeList(notes)
        const decorated = []
        for (let index = 0; index < source.length; ++index)
            decorated.push({ "item": source[index], "index": index })
        decorated.sort(function(left, right) {
            const leftOrder = left.item && Number.isFinite(Number(left.item.order))
                            ? Number(left.item.order) : Number.MAX_SAFE_INTEGER
            const rightOrder = right.item && Number.isFinite(Number(right.item.order))
                             ? Number(right.item.order) : Number.MAX_SAFE_INTEGER
            return leftOrder === rightOrder ? left.index - right.index : leftOrder - rightOrder
        })
        const result = []
        for (let index = 0; index < decorated.length; ++index)
            result.push(decorated[index].item)
        return result
    }

    function termSegments() {
        const result = []
        const source = root.safeList(root.terms)
        for (let index = 0; index < source.length; ++index) {
            const term = source[index] || ({})
            result.push({
                "label": root.stringValue(term.name) || "第 " + (index + 1) + " 学期",
                "value": String(index)
            })
        }
        return result
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        contentHeight: contentColumn.implicitHeight + Theme.pagePadding * 2

        ColumnLayout {
            id: contentColumn
            x: Math.max(Theme.pagePadding,
                        (scrollView.availableWidth - width) / 2)
            y: Theme.pagePadding
            width: Math.max(0, Math.min(scrollView.availableWidth - Theme.pagePadding * 2,
                                        1120))
            spacing: Theme.sectionGap

            SegmentedControl {
                Layout.fillWidth: root.compact
                Layout.preferredWidth: Math.min(implicitWidth, contentColumn.width)
                Layout.maximumWidth: contentColumn.width
                visible: root.terms.length > 1
                model: root.termSegments()
                value: String(root.selectedTermIndex)
                onActivated: function(newValue) {
                    const index = Number(newValue)
                    if (Number.isInteger(index) && index >= 0 && index < root.terms.length)
                        root.selectedTermIndex = index
                }
            }

            Card {
                Layout.fillWidth: true
                visible: root.terms.length === 0
                implicitHeight: emptyLayout.implicitHeight + Theme.cardPadding * 2
                elevation: 0

                ColumnLayout {
                    id: emptyLayout
                    anchors.fill: parent
                    anchors.margins: Theme.cardPadding
                    spacing: Theme.spacing4

                    Text {
                        Layout.fillWidth: true
                        text: "暂无结构化校历"
                        font.pixelSize: Theme.fontSection
                        font.weight: Theme.weightStrong
                        color: Theme.text
                    }
                    Text {
                        Layout.fillWidth: true
                        text: "当前校历没有可展示的学期数据"
                        font.pixelSize: Theme.fontBody
                        color: Theme.mutedText
                        wrapMode: Text.WordWrap
                    }
                }
            }

            Card {
                Layout.fillWidth: true
                visible: root.terms.length > 0
                implicitHeight: overviewLayout.implicitHeight + Theme.cardPadding * 2

                GridLayout {
                    id: overviewLayout
                    anchors.fill: parent
                    anchors.margins: Theme.cardPadding
                    columns: root.compact ? 1 : 2
                    columnSpacing: Theme.spacing12
                    rowSpacing: Theme.spacing12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacing4

                        Text {
                            Layout.fillWidth: true
                            text: root.stringValue(root.calendarData.title)
                                  || root.stringValue(root.calendarData.academicYear)
                                  || "学年校历"
                            font.pixelSize: Theme.fontSection
                            font.weight: Theme.weightStrong
                            color: Theme.text
                            wrapMode: Text.WordWrap
                        }
                        Text {
                            Layout.fillWidth: true
                            text: (root.stringValue(root.selectedTerm.name) || "当前学期")
                                  + " · "
                                  + root.dateRange(root.selectedTerm.startDate,
                                                   root.selectedTerm.endDate)
                            font.pixelSize: Theme.fontLabel
                            color: Theme.mutedText
                            wrapMode: Text.WordWrap
                        }
                    }

                    AppButton {
                        Layout.alignment: root.compact ? Qt.AlignLeft
                                                      : Qt.AlignRight | Qt.AlignVCenter
                        text: "查看原版"
                        type: "secondary"
                        enabled: root.stringValue(root.selectedTerm.sourceImageUrl).length > 0
                        onClicked: root.viewOriginalRequested(
                            root.stringValue(root.selectedTerm.sourceImageUrl))
                    }
                }
            }

            SectionHeader {
                Layout.fillWidth: true
                visible: root.terms.length > 0
                title: "教学周次"
                description: root.safeList(root.selectedTerm.weeks).length + " 周"
            }

            GridLayout {
                Layout.fillWidth: true
                visible: root.terms.length > 0
                         && root.safeList(root.selectedTerm.weeks).length > 0
                columns: contentColumn.width >= 900 ? 4
                         : (contentColumn.width >= 520 ? 2 : 1)
                columnSpacing: Theme.spacing12
                rowSpacing: Theme.spacing12

                Repeater {
                    model: root.safeList(root.selectedTerm.weeks)

                    delegate: Card {
                        id: weekCard
                        required property var modelData

                        Layout.fillWidth: true
                        implicitHeight: weekLayout.implicitHeight + Theme.spacing12 * 2
                        color: root.phaseBackground(weekCard.modelData
                                                    ? weekCard.modelData.phase : "")
                        elevation: 0

                        ColumnLayout {
                            id: weekLayout
                            anchors.fill: parent
                            anchors.margins: Theme.spacing12
                            spacing: Theme.spacing4

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.spacing8

                                Text {
                                    Layout.fillWidth: true
                                    text: "第 " + root.stringValue(
                                        weekCard.modelData ? weekCard.modelData.weekNo : "") + " 周"
                                    font.pixelSize: Theme.fontBody
                                    font.weight: Theme.weightStrong
                                    color: Theme.text
                                }
                                Text {
                                    text: root.stringValue(weekCard.modelData
                                                           ? weekCard.modelData.label : "")
                                          || root.phaseLabel(weekCard.modelData
                                                             ? weekCard.modelData.phase : "")
                                    font.pixelSize: Theme.fontCaption
                                    font.weight: Theme.weightMedium
                                    color: root.phaseForeground(weekCard.modelData
                                                                ? weekCard.modelData.phase : "")
                                }
                            }
                            Text {
                                Layout.fillWidth: true
                                text: root.dateRange(weekCard.modelData
                                                     ? weekCard.modelData.startDate : undefined,
                                                     weekCard.modelData
                                                     ? weekCard.modelData.endDate : undefined)
                                font.pixelSize: Theme.fontCaption
                                color: Theme.mutedText
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                visible: root.terms.length > 0
                         && root.safeList(root.selectedTerm.weeks).length === 0
                text: "本学期暂无周次安排"
                font.pixelSize: Theme.fontBody
                color: Theme.mutedText
            }

            SectionHeader {
                Layout.fillWidth: true
                visible: root.terms.length > 0
                title: "重要事项"
                description: root.sortedEvents(root.selectedTerm.events).length + " 项"
            }

            ColumnLayout {
                Layout.fillWidth: true
                visible: root.terms.length > 0
                         && root.sortedEvents(root.selectedTerm.events).length > 0
                spacing: Theme.spacing8

                Repeater {
                    model: root.sortedEvents(root.selectedTerm.events)

                    delegate: Card {
                        id: eventCard
                        required property var modelData

                        Layout.fillWidth: true
                        implicitHeight: eventLayout.implicitHeight + Theme.spacing12 * 2
                        elevation: 0

                        RowLayout {
                            id: eventLayout
                            anchors.fill: parent
                            anchors.margins: Theme.spacing12
                            spacing: Theme.spacing12

                            Rectangle {
                                Layout.preferredWidth: 4
                                Layout.fillHeight: true
                                radius: 2
                                color: Theme.accent
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: Theme.spacing4

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: Theme.spacing8

                                    Text {
                                        Layout.fillWidth: true
                                        text: root.stringValue(eventCard.modelData
                                                               ? eventCard.modelData.title : "")
                                              || "未命名事项"
                                        font.pixelSize: Theme.fontBody
                                        font.weight: Theme.weightStrong
                                        color: Theme.text
                                        wrapMode: Text.WordWrap
                                    }
                                    Text {
                                        text: root.eventTypeLabel(eventCard.modelData
                                                                  ? eventCard.modelData.type : "")
                                        font.pixelSize: Theme.fontCaption
                                        font.weight: Theme.weightMedium
                                        color: Theme.accent
                                    }
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: root.dateRange(eventCard.modelData
                                                         ? eventCard.modelData.startDate : undefined,
                                                         eventCard.modelData
                                                         ? eventCard.modelData.endDate : undefined)
                                    font.pixelSize: Theme.fontCaption
                                    color: Theme.mutedText
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                visible: root.terms.length > 0
                         && root.sortedEvents(root.selectedTerm.events).length === 0
                text: "本学期暂无重要事项"
                font.pixelSize: Theme.fontBody
                color: Theme.mutedText
            }

            SectionHeader {
                Layout.fillWidth: true
                visible: root.terms.length > 0
                title: "校历说明"
                description: root.sortedNotes(root.selectedTerm.notes).length + " 条"
            }

            Card {
                id: notesCard
                property bool expanded: false

                Layout.fillWidth: true
                visible: root.terms.length > 0
                         && root.sortedNotes(root.selectedTerm.notes).length > 0
                implicitHeight: notesLayout.implicitHeight + Theme.cardPadding * 2
                elevation: 0

                ColumnLayout {
                    id: notesLayout
                    anchors.fill: parent
                    anchors.margins: Theme.cardPadding
                    spacing: Theme.spacing12

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacing8

                        Text {
                            Layout.fillWidth: true
                            text: notesCard.expanded ? "收起全部说明" : "展开全部说明"
                            font.pixelSize: Theme.fontBody
                            font.weight: Theme.weightStrong
                            color: Theme.text
                        }
                        Text {
                            text: notesCard.expanded ? "⌃" : "⌄"
                            font.pixelSize: Theme.fontSection
                            color: Theme.mutedText
                        }
                    }

                    Repeater {
                        model: root.sortedNotes(root.selectedTerm.notes)

                        delegate: RowLayout {
                            id: noteRow
                            required property int index
                            required property var modelData

                            Layout.fillWidth: true
                            visible: notesCard.expanded
                            spacing: Theme.spacing8

                            Text {
                                Layout.alignment: Qt.AlignTop
                                text: root.stringValue(noteRow.modelData
                                                       ? noteRow.modelData.order : "")
                                      || String(noteRow.index + 1)
                                font.pixelSize: Theme.fontCaption
                                font.weight: Theme.weightStrong
                                color: Theme.accent
                            }
                            Text {
                                Layout.fillWidth: true
                                text: root.stringValue(noteRow.modelData
                                                       ? noteRow.modelData.text : "")
                                font.pixelSize: Theme.fontBody
                                color: Theme.text
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }

                TapHandler {
                    onTapped: notesCard.expanded = !notesCard.expanded
                }
            }

            Text {
                Layout.fillWidth: true
                visible: root.terms.length > 0
                         && root.sortedNotes(root.selectedTerm.notes).length === 0
                text: "本学期暂无补充说明"
                font.pixelSize: Theme.fontBody
                color: Theme.mutedText
            }
        }
    }
}
