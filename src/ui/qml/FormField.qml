import QtQuick
import QtQuick.Layouts
import Gittyup

// A labeled form field. The content fills the width.
ColumnLayout {
    id: root

    property string label
    // Explains a problem with the value.
    property string error
    property string hint

    default property alias content: holder.data

    Layout.fillWidth: true
    spacing: 6

    Text {
        visible: root.label !== ""
        text: root.label
        color: Theme.textMuted
        font.pixelSize: 12
        font.weight: Font.DemiBold
    }

    ColumnLayout {
        id: holder

        Layout.fillWidth: true
        spacing: 6
    }

    Text {
        Layout.fillWidth: true
        visible: root.error !== "" || root.hint !== ""
        text: root.error !== "" ? root.error : root.hint
        wrapMode: Text.Wrap
        color: root.error !== "" ? Theme.deleted : Theme.textMuted
        font.pixelSize: 12
    }
}
