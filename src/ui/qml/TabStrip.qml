import QtQuick
import Gittyup

// Repository tabs above the tool bar. 'tabStrip' is the C++ TabStrip.
Rectangle {
    id: root

    readonly property int count: tabStrip.tabs.length + (tabStrip.welcome ? 1 : 0)
    readonly property real tabWidth: {
        const available = width - 12 - addButton.width - 8
        return Math.max(96, Math.min(220, available / Math.max(1, count)))
    }

    color: Theme.base

    // The line under the tabs. The current tab covers it.
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: Theme.border
    }

    component Tab: Item {
        id: tab

        property string name
        property string tip
        property string icon: "repo"
        property bool current: false
        // Hide the separator before the current tab and the one after it.
        property bool separator: true
        // The position of a tab that can be dragged to another position.
        property int tabIndex: -1

        property real dragOffset: 0
        readonly property bool dragging: dragOffset !== 0

        signal activated()
        signal closed()
        signal menuRequested(real x, real y)
        signal moveRequested(int to)

        width: root.tabWidth
        height: parent.height
        clip: true
        z: dragging ? 1 : 0
        transform: Translate { x: tab.dragOffset }

        Rectangle {
            anchors.fill: parent
            anchors.bottomMargin: -radius
            radius: 7
            color: tab.current ? Theme.toolbar
                               : mouse.containsMouse ? Qt.rgba(Theme.hover.r, Theme.hover.g,
                                                               Theme.hover.b, 0.6)
                                                     : "transparent"
            border.color: tab.current ? Theme.border : "transparent"
            Behavior on color { ColorAnimation { duration: 80 } }
        }

        // Accent line on top of the current tab.
        Rectangle {
            visible: tab.current
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width - 16
            height: 2
            radius: 1
            color: Theme.accent
        }

        Rectangle {
            visible: tab.separator && !tab.current && !mouse.containsMouse
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: 1
            height: 16
            color: Theme.border
        }

        MouseArea {
            id: mouse

            // The x of the press in the parent of the tab.
            property real pressX
            property bool moved: false

            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
            onPressed: (event) => {
                pressX = mapToItem(tab.parent, event.x, 0).x
                moved = false
            }
            onPositionChanged: (event) => {
                if (!pressed || tab.tabIndex < 0 || !(pressedButtons & Qt.LeftButton))
                    return
                const offset = mapToItem(tab.parent, event.x, 0).x - pressX
                if (!moved && Math.abs(offset) < 6)
                    return
                moved = true
                const count = tabStrip.tabs.length
                // Keep the tab in the strip.
                const min = -tab.tabIndex * root.tabWidth
                const max = (count - 1 - tab.tabIndex) * root.tabWidth
                tab.dragOffset = Math.max(min, Math.min(max, offset))
            }
            onReleased: {
                if (moved) {
                    const to = tab.tabIndex + Math.round(tab.dragOffset / root.tabWidth)
                    tab.dragOffset = 0
                    if (to !== tab.tabIndex)
                        tab.moveRequested(to)
                }
            }
            onCanceled: tab.dragOffset = 0
            onClicked: (event) => {
                if (moved)
                    return
                if (event.button === Qt.MiddleButton) {
                    tab.closed()
                } else if (event.button === Qt.RightButton) {
                    const p = mapToItem(null, event.x, event.y)
                    tab.menuRequested(p.x, p.y)
                } else {
                    tab.activated()
                }
            }
            onExited: host.hideToolTip()
        }

        HoverTip {
            target: tab
            text: tab.tip
            hovered: mouse.containsMouse && !closeMouse.containsMouse
        }

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.right: closeButton.left
            anchors.rightMargin: 4
            anchors.verticalCenter: parent.verticalCenter
            spacing: 7

            Icon {
                id: tabIcon

                anchors.verticalCenter: parent.verticalCenter
                name: tab.icon
                size: 14
                color: tab.current ? Theme.accent : Theme.textMuted
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - tabIcon.width - parent.spacing
                text: tab.name
                elide: Text.ElideMiddle
                color: tab.current ? Theme.text : Theme.textMuted
                font.pixelSize: 12
                font.weight: tab.current ? Font.DemiBold : Font.Normal
            }
        }

        Rectangle {
            id: closeButton

            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: 18
            height: 18
            radius: 4
            opacity: tab.current || mouse.containsMouse || closeMouse.containsMouse ? 1 : 0
            color: closeMouse.pressed ? Theme.pressed
                                      : closeMouse.containsMouse ? Theme.hover : "transparent"

            Icon {
                anchors.centerIn: parent
                name: "close"
                size: 10
                color: closeMouse.containsMouse ? Theme.text : Theme.textMuted
            }

            MouseArea {
                id: closeMouse

                anchors.fill: parent
                hoverEnabled: true
                onClicked: tab.closed()
            }
        }
    }

    Row {
        id: row

        anchors.left: parent.left
        anchors.leftMargin: 6
        anchors.top: parent.top
        anchors.topMargin: 5
        anchors.bottom: parent.bottom

        Repeater {
            model: tabStrip.tabs

            delegate: Tab {
                required property int index
                required property var modelData

                name: modelData.name
                tip: modelData.path
                tabIndex: index
                current: !tabStrip.welcome && index === tabStrip.current
                separator: tabStrip.welcome || index + 1 !== tabStrip.current
                onActivated: tabStrip.selectTab(index)
                onClosed: tabStrip.closeTab(index)
                onMenuRequested: (x, y) => tabStrip.showMenu(index, x, y)
                onMoveRequested: (to) => tabStrip.moveTab(index, to)
            }
        }

        // The page to open a repository in a new tab.
        Tab {
            visible: tabStrip.welcome
            name: qsTr("New Tab")
            icon: "plus"
            current: true
            onClosed: tabStrip.closeWelcome()
        }

        Item {
            width: 4
            height: parent.height
        }

        ActionButton {
            id: addButton

            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: -2
            compact: true
            implicitWidth: 26
            implicitHeight: 26
            icon: "plus"
            tip: qsTr("New tab")
            onClicked: tabStrip.newTab()
        }
    }
}
