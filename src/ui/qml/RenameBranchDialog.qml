import QtQuick
import QtQuick.Layouts
import Gittyup

// 'dialog' is the C++ RenameBranchDialog.
DialogPage {
    initialFocus: renameField
    title: qsTr("Rename branch")
    subtitle: qsTr("Give '%1' a new name.").arg(dialog.oldName)
    acceptText: qsTr("Rename Branch")
    acceptEnabled: dialog.acceptable

    FormField {
        label: qsTr("New name")
        error: dialog.nameError

        TextField {
            id: renameField

            Layout.fillWidth: true
            text: dialog.name
            error: dialog.nameError !== ""
            onTextEdited: dialog.name = text
            Component.onCompleted: {
                forceActiveFocus()
                selectAll()
            }
        }
    }
}
