import QtQuick 2.15
import QtQuick.Effects
import "../styles"

// Fluent 卡片：圆角表面 + 细描边 + 柔和投影（elevation）。
// 结构说明：root 为透明 Item；投影/表面放在负 z（位于内容之下），
// 调用方内容作为普通子元素（z≥0）叠在表面之上，因此无需 default 属性别名，
// 也就避免了别名把卡片自身骨架误当作内容的坑。
//   Card { Layout.fillWidth: true; implicitHeight: col.implicitHeight + 32
//          ColumnLayout { id: col; anchors.fill: parent; anchors.margins: 16 } }
Item {
    id: root

    property int radius: Theme.cardRadius
    property color color: Theme.surface
    property bool bordered: true
    property real elevation: 1.0   // 0 = 平贴；1 = 标准卡片；>1 = 更高悬浮

    // 隐藏投射体：给 MultiEffect 作为阴影源。
    Rectangle {
        id: caster
        z: -3
        anchors.fill: parent
        radius: root.radius
        color: "#000000"
        visible: false
    }

    // 柔和投影，位于表面之后；溢出到卡片外的部分即为 elevation。
    MultiEffect {
        z: -2
        source: caster
        anchors.fill: caster
        visible: root.elevation > 0
        autoPaddingEnabled: true
        shadowEnabled: true
        shadowColor: Theme.shadow
        shadowBlur: 0.9
        shadowScale: 1.0
        shadowHorizontalOffset: 0
        shadowVerticalOffset: Math.round(2 * root.elevation)
    }

    // 卡片表面。
    Rectangle {
        id: surface
        z: -1
        anchors.fill: parent
        radius: root.radius
        color: root.color
        border.width: root.bordered ? 1 : 0
        border.color: Theme.border
    }
}
