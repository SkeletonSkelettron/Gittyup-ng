import QtQuick
import QtQuick.Layouts
import Gittyup

// A labeled setting. The controls are laid out in a column on the right.
RowLayout {
    id: root

    property string label
    property string hint

    default property alias content: controls.data

    Layout.fillWidth: true
    spacing: 16

    ColumnLayout {
        Layout.preferredWidth: 190
        Layout.maximumWidth: 190
        Layout.alignment: Qt.AlignTop
        Layout.topMargin: 7
        spacing: 2

        Text {
            Layout.fillWidth: true
            text: root.label
            wrapMode: Text.Wrap
            color: Theme.text
            font.pixelSize: 13
        }

        Text {
            Layout.fillWidth: true
            visible: root.hint !== ""
            text: root.hint
            wrapMode: Text.Wrap
            color: Theme.textMuted
            font.pixelSize: 11
        }
    }

    ColumnLayout {
        id: controls

        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop
        spacing: 10
    }
}
