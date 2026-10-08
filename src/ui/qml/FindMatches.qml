import QtQuick

// The matches of the find bar behind a line of text in a monospace font.
// Each match has a 'start' and a 'length' in characters and whether it's
// the 'current' one.
Item {
    id: root

    property var matches: []
    property real charWidth

    anchors.fill: parent

    Repeater {
        model: root.matches

        delegate: Rectangle {
            required property var modelData

            x: modelData.start * root.charWidth - 1
            y: 2
            width: modelData.length * root.charWidth + 2
            height: root.height - 4
            radius: 3
            color: modelData.current ? "#F2994A" : "#F2C94C"
            opacity: modelData.current ? 0.75 : 0.35
        }
    }
}
