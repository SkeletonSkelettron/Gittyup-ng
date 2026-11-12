import QtQuick
import QtQuick.Layouts
import Gittyup

// The menu bar of a window, unless the platform shows the menus. 'target'
// is the C++ window, which has 'menuBarVisible', 'menuTitles' and
// showMenu(). When 'frame', the WindowFrame of the window, is custom, it's
// also the title bar: the title is in the middle, the buttons of the window
// on the right, and the empty space moves the window.
Rectangle {
    id: root

    property var target
    property var frame
    readonly property bool titleBar: !!frame && frame.custom
    readonly property color glyphColor: titleBar && !frame.active ? Theme.textMuted
                                                                  : Theme.text

    Layout.preferredHeight: implicitHeight
    implicitHeight: titleBar ? 32 : 28
    visible: target.menuBarVisible || titleBar
    color: Theme.base

    // Drag the window by the empty space, and maximize it with a double
    // click. It's under the menus and the buttons, which take their clicks.
    Item {
        anchors.fill: parent
        enabled: root.titleBar

        DragHandler {
            target: null
            onActiveChanged: {
                if (active)
                    root.frame.startMove()
            }
        }

        TapHandler {
            onDoubleTapped: root.frame.toggleMaximized()
        }
    }

    // The menus stop before the buttons when the window is narrow.
    Row {
        id: menus

        anchors.left: parent.left
        anchors.leftMargin: 6
        anchors.verticalCenter: parent.verticalCenter
        width: Math.min(implicitWidth, (root.titleBar ? buttons.x : root.width) - x)
        height: implicitHeight
        clip: true
        visible: root.target.menuBarVisible

        Repeater {
            model: root.target.menuTitles

            delegate: Rectangle {
                id: title

                required property int index
                required property string modelData

                width: label.implicitWidth + 16
                height: 24
                radius: 5
                color: titleMouse.pressed || titleMouse.containsMouse ? Theme.hover
                                                                      : "transparent"

                Text {
                    id: label

                    anchors.centerIn: parent
                    text: title.modelData
                    color: Theme.text
                    font.pixelSize: 13
                }

                MouseArea {
                    id: titleMouse

                    anchors.fill: parent
                    hoverEnabled: true
                    onPressed: {
                        const p = title.mapToItem(null, 0, title.height + 2)
                        root.target.showMenu(title.index, p.x, p.y)
                    }
                }
            }
        }
    }

    // The title between the menus and the buttons: in the middle of the
    // window when there's room, and shortened when it doesn't fit.
    Text {
        readonly property real start: (menus.visible ? menus.x + menus.width : 0) + 16
        readonly property real end: buttons.x - 16

        x: Math.max(start, Math.min((parent.width - width) / 2, end - width))
        anchors.verticalCenter: parent.verticalCenter
        width: Math.max(0, Math.min(implicitWidth, end - start))
        visible: root.titleBar && width > 40
        text: root.titleBar ? root.frame.title : ""
        color: root.frame && root.frame.active ? Theme.text : Theme.textMuted
        font.pixelSize: 12
        elide: Text.ElideMiddle
    }

    // Minimize, maximize or restore, and close.
    Row {
        id: buttons

        anchors.right: parent.right
        height: parent.height
        visible: root.titleBar

        WindowButton {
            onClicked: root.frame.minimize()

            Rectangle {
                anchors.centerIn: parent
                width: 10
                height: 1
                color: root.glyphColor
            }
        }

        WindowButton {
            id: maximize

            onClicked: root.frame.toggleMaximized()

            // A square, or two when the window is maximized.
            Item {
                anchors.centerIn: parent
                width: 10
                height: 10

                Rectangle {
                    visible: !!root.frame && root.frame.maximized
                    x: 2
                    width: 8
                    height: 8
                    color: "transparent"
                    border.width: 1
                    border.color: root.glyphColor
                }

                Rectangle {
                    readonly property bool restore: !!root.frame && root.frame.maximized

                    y: restore ? 2 : 0
                    width: restore ? 8 : 10
                    height: restore ? 8 : 10
                    // Covers the square behind it.
                    color: Qt.tint(Theme.base, maximize.color)
                    border.width: 1
                    border.color: root.glyphColor
                }
            }
        }

        WindowButton {
            id: closeButton

            danger: true
            onClicked: root.frame.close()

            Repeater {
                model: [45, -45]

                Rectangle {
                    required property real modelData

                    anchors.centerIn: parent
                    width: 14
                    height: 1.3
                    rotation: modelData
                    antialiasing: true
                    color: closeButton.hovered ? Theme.alertText : root.glyphColor
                }
            }
        }
    }

    // A button of the window, with its glyph as its children. Closing is red.
    component WindowButton: Rectangle {
        id: button

        property bool danger: false
        readonly property bool hovered: buttonMouse.containsMouse

        signal clicked()

        width: 46
        height: parent.height
        color: {
            if (!buttonMouse.containsMouse)
                return "transparent"
            if (danger)
                return Theme.alert
            return buttonMouse.pressed ? Theme.pressed : Theme.hover
        }

        MouseArea {
            id: buttonMouse

            anchors.fill: parent
            hoverEnabled: true
            onClicked: button.clicked()
        }
    }
}
