import QtQuick
import QtQuick.Layouts
import Gittyup

// 'dialog' is the C++ CommitDialog.
DialogPage {
    title: dialog.title
    acceptText: dialog.acceptText
    rejectText: dialog.rejectText

    TextArea {
        Layout.fillWidth: true
        implicitHeight: 130
        text: dialog.message
        font.family: Theme.monoFont
        onTextEdited: dialog.message = text
    }

    CheckBox {
        text: dialog.promptText
        checked: dialog.prompt
        onToggled: dialog.prompt = checked
    }
}
