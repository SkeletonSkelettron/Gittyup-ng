import QtQuick
import QtQuick.Layouts
import Gittyup

// A group of settings with a title.
ColumnLayout {
    id: root

    property string title

    default property alias content: rows.data

    Layout.fillWidth: true
    spacing: 12

    Text {
        visible: root.title !== ""
        text: root.title
        color: Theme.textMuted
        font.pixelSize: 11
        font.weight: Font.DemiBold
        font.letterSpacing: 0.8
        font.capitalization: Font.AllUppercase
    }

    ColumnLayout {
        id: rows

        Layout.fillWidth: true
        spacing: 14
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.topMargin: 6
        height: 1
        color: Theme.border
    }
}
