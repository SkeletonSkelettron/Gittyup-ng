import QtQuick
import QtQuick.Layouts
import Gittyup

// 'dialog' is the C++ IgnoreDialog.
DialogPage {
    title: qsTr("Ignore files")
    subtitle: qsTr("The patterns are added to .gitignore, one per line.")
    acceptText: qsTr("Ignore")
    acceptEnabled: dialog.pattern.trim() !== ""
    initialFocus: patternArea.textArea

    TextArea {
        id: patternArea

        Layout.fillWidth: true
        implicitHeight: 140
        text: dialog.pattern
        font.family: Theme.monoFont
        onTextEdited: dialog.pattern = text
    }
}
