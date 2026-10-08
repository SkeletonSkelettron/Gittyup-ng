import QtQuick
import QtQuick.Layouts
import Gittyup

// A toolbar button with an icon above a short label. Buttons with a menu
// show a chevron that opens it; right-click or press-and-hold opens it too.
Item {
    id: root

    property string icon
    property string text
    property string tip
    property bool checked: false
    property bool compact: false
    property bool hasMenu: false
    property bool menuOnly: false
    property bool showChevron: true
    property int badge: 0
    property color badgeColor: Theme.badge
    // Draw the icon in this color instead, unless it's transparent.
    property color foregroundOverride: "transparent"

    readonly property bool hovered: mouse.containsMouse || chevronMouse.containsMouse
    readonly property color foreground: !enabled ? Theme.textDisabled
                                                 : foregroundOverride.a > 0 ? foregroundOverride
                                                 : checked ? Theme.accent : Theme.text

    signal clicked()
    // Scene coordinates of the point where the menu should open.
    signal menuRequested(real x, real y)

    implicitWidth: compact ? 32 : Math.max(46, content.implicitWidth + 12)
    implicitHeight: compact ? 32 : 46
    // Never squeeze buttons in a layout.
    Layout.minimumWidth: implicitWidth

    function requestMenu() {
        const p = root.mapToItem(null, 0, root.height + 2)
        root.menuRequested(p.x, p.y)
    }

    Rectangle {
        anchors.fill: parent
        radius: 6
        color: mouse.pressed ? Theme.pressed
                             : root.hovered ? Theme.hover
                                            : root.checked ? Qt.rgba(Theme.accent.r, Theme.accent.g,
                                                                     Theme.accent.b, 0.14)
                                                           : "transparent"
        Behavior on color { ColorAnimation { duration: 80 } }
    }

    MouseArea {
        id: mouse

        property bool held: false

        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        pressAndHoldInterval: 450
        cursorShape: Qt.PointingHandCursor

        onPressed: held = false
        onPressAndHold: {
            if (root.hasMenu) {
                held = true
                root.requestMenu()
            }
        }
        onClicked: (event) => {
            if (held)
                return
            if (event.button === Qt.RightButton) {
                if (root.hasMenu)
                    root.requestMenu()
            } else if (root.menuOnly) {
                root.requestMenu()
            } else {
                root.clicked()
            }
        }
        onExited: host.hideToolTip()
    }

    Timer {
        interval: 600
        running: root.tip !== "" && root.hovered && !mouse.pressed
        onTriggered: {
            const p = root.mapToItem(null, 0, 0)
            host.showToolTip(root.tip, p.x, p.y, root.width, root.height)
        }
    }

    Column {
        id: content

        anchors.centerIn: parent
        spacing: 3

        Icon {
            anchors.horizontalCenter: parent.horizontalCenter
            name: root.icon
            size: root.compact ? 18 : 19
            color: root.foreground
        }

        Row {
            visible: !root.compact
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 1

            Text {
                text: root.text
                color: root.enabled ? Theme.textMuted : Theme.textDisabled
                font.pixelSize: 11
            }

            Icon {
                id: chevron

                visible: root.hasMenu && root.showChevron && !root.menuOnly
                anchors.verticalCenter: parent.verticalCenter
                name: "chevron-down"
                size: 11
                color: chevronMouse.containsMouse ? Theme.accent : Theme.textMuted

                MouseArea {
                    id: chevronMouse

                    anchors.fill: parent
                    anchors.margins: -5
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.requestMenu()
                }
            }
        }
    }

    Rectangle {
        visible: root.badge > 0 && root.enabled
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: root.compact ? -2 : 1
        anchors.rightMargin: root.compact ? -2 : 3
        height: 15
        radius: height / 2
        width: Math.max(height, badgeLabel.implicitWidth + 8)
        color: root.badgeColor

        Text {
            id: badgeLabel

            anchors.centerIn: parent
            text: root.badge > 999 ? "999+" : root.badge
            color: Theme.badgeText
            font.pixelSize: 10
            font.bold: true
        }
    }
}
