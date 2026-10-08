import QtQuick
import QtQuick.Layouts
import Gittyup

// A caption above a bold value, e.g. the current repository or branch.
Item {
    id: root

    property string caption
    property string value
    property string icon
    property string tip
    property bool showChevron: true
    property real maxTextWidth: 180

    signal clicked()

    // Space taken by everything but the text.
    readonly property real chrome: 16 + (icon !== "" ? 18 + row.spacing : 0)
                                   + (chevron.visible ? chevron.width + row.spacing : 0)
    readonly property real textWidth: Math.min(Math.max(captionLabel.implicitWidth,
                                                        valueLabel.implicitWidth),
                                               maxTextWidth)

    implicitWidth: chrome + textWidth
    implicitHeight: 40
    // Shrink and elide the text when the tool bar gets crowded.
    Layout.fillWidth: true
    Layout.minimumWidth: Math.min(implicitWidth,
                                  chrome + Math.max(captionLabel.implicitWidth, 40))
    Layout.maximumWidth: implicitWidth

    Rectangle {
        anchors.fill: parent
        radius: 6
        color: mouse.pressed ? Theme.pressed : mouse.containsMouse ? Theme.hover : "transparent"
        Behavior on color { ColorAnimation { duration: 80 } }
    }

    MouseArea {
        id: mouse

        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
        onExited: host.hideToolTip()
    }

    Timer {
        interval: 600
        running: root.tip !== "" && mouse.containsMouse && !mouse.pressed
        onTriggered: {
            const p = root.mapToItem(null, 0, 0)
            host.showToolTip(root.tip, p.x, p.y, root.width, root.height)
        }
    }

    Row {
        id: row

        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        Icon {
            anchors.verticalCenter: parent.verticalCenter
            visible: root.icon !== ""
            name: root.icon
            size: 18
            color: root.enabled ? Theme.accent : Theme.textDisabled
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 1
            width: Math.max(0, Math.min(root.textWidth, root.width - root.chrome))

            Text {
                id: captionLabel

                width: parent.width
                text: root.caption
                color: Theme.textMuted
                font.pixelSize: 10
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 0.6
                elide: Text.ElideRight
            }

            Text {
                id: valueLabel

                width: parent.width
                text: root.value
                color: root.enabled ? Theme.text : Theme.textDisabled
                font.pixelSize: 13
                font.bold: true
                elide: Text.ElideMiddle
            }
        }

        Icon {
            id: chevron

            anchors.verticalCenter: parent.verticalCenter
            visible: root.showChevron && root.enabled
            name: "chevron-down"
            size: 12
            color: Theme.textMuted
        }
    }
}
