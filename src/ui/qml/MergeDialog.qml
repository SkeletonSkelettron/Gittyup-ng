import QtQuick
import QtQuick.Layouts
import Gittyup

// 'dialog' is the C++ MergeDialog.
DialogPage {
    initialFocus: refCombo
    title: dialog.buttonText
    subtitle: dialog.labelText
    acceptText: dialog.buttonText
    acceptEnabled: dialog.acceptable

    FormField {
        label: qsTr("Reference")

        ComboBox {
            id: refCombo

            Layout.fillWidth: true
            model: dialog.refs
            textRole: "text"
            currentIndex: dialog.refIndex
            onActivated: (index) => dialog.refIndex = index
        }
    }

    FormField {
        label: qsTr("Action")

        ComboBox {
            Layout.fillWidth: true
            model: dialog.actions
            currentIndex: dialog.action
            onActivated: (index) => dialog.action = index
        }
    }

    CheckBox {
        visible: dialog.noCommitVisible
        text: qsTr("Don't commit the merge yet")
        checked: dialog.noCommit
        onToggled: dialog.noCommit = checked
    }
}
