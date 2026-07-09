import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// Week switcher component: previous / current / next week navigation

RowLayout {
    id: weekSwitcher

    property int currentWeek: 1
    property int totalWeeks: 20

    signal previousWeek()
    signal nextWeek()
    signal goToCurrentWeek()

    spacing: 4

    ToolButton {
        text: "◀"
        font.pixelSize: 14
        onClicked: previousWeek()
        ToolTip.text: "上一周"
        ToolTip.visible: hovered
    }

    Label {
        text: "第 " + currentWeek + " 周"
        font.pixelSize: 15
        font.bold: true
        Layout.preferredWidth: 100
        horizontalAlignment: Text.AlignHCenter
    }

    ToolButton {
        text: "▶"
        font.pixelSize: 14
        onClicked: nextWeek()
        ToolTip.text: "下一周"
        ToolTip.visible: hovered
    }

    ToolButton {
        text: "返回本周"
        font.pixelSize: 11
        onClicked: goToCurrentWeek()
    }

    Label {
        text: "/ 共 " + totalWeeks + " 周"
        font.pixelSize: 12
        color: "#999"
    }
}
