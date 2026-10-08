import QtQuick
import QtQuick.Controls.Basic as Controls
import Gittyup

// A multi-line text field that scrolls.
Rectangle {
    id: root

    property alias text: area.text
    property alias placeholderText: area.placeholderText
    property alias font: area.font
    property alias textArea: area

    signal textEdited()

    implicitHeight: 96
    radius: 6
    color: Theme.field
    border.width: area.activeFocus ? 2 : 1
    border.color: area.activeFocus ? Theme.accent : Theme.border
    opacity: enabled ? 1 : 0.6

    Controls.ScrollView {
        id: scrollView

        anchors.fill: parent
        anchors.margins: 1

        // ScrollView doesn't place a custom scroll bar.
        Controls.ScrollBar.vertical: ThinScrollBar {
            parent: scrollView
            x: scrollView.width - width - 2
            y: 2
            height: scrollView.height - 4
            thickness: 6
        }

        Controls.TextArea {
            id: area

            leftPadding: 10
            rightPadding: 10
            topPadding: 8
            bottomPadding: 8
            wrapMode: TextEdit.Wrap
            color: Theme.text
            placeholderTextColor: Theme.textMuted
            selectionColor: Theme.accent
            selectedTextColor: Theme.accentText
            selectByMouse: true
            font.pixelSize: 13
            background: null
            onTextChanged: if (activeFocus) root.textEdited()
        }
    }
}
