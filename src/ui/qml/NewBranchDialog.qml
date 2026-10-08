import QtQuick
import QtQuick.Layouts
import Gittyup

// 'dialog' is the C++ NewBranchDialog.
DialogPage {
    initialFocus: nameField
    title: qsTr("Create a new branch")
    subtitle: dialog.commitText !== "" ? qsTr("The branch starts at %1.").arg(dialog.commitText)
                                       : ""
    acceptText: qsTr("Create Branch")
    acceptEnabled: dialog.acceptable

    FormField {
        label: qsTr("Branch name")
        error: dialog.nameError

        TextField {
            id: nameField

            Layout.fillWidth: true
            placeholderText: qsTr("feature/my-change")
            text: dialog.name
            error: dialog.nameError !== ""
            focus: true
            onTextEdited: dialog.name = text
        }
    }

    FormField {
        visible: dialog.commitText === ""
        label: qsTr("Start point")

        ComboBox {
            Layout.fillWidth: true
            model: dialog.startPoints
            textRole: "text"
            currentIndex: dialog.startPoint
            onActivated: (index) => dialog.startPoint = index
        }
    }

    FormField {
        label: qsTr("Upstream")
        hint: qsTr("The remote branch to track, for pull and push.")

        ComboBox {
            Layout.fillWidth: true
            model: dialog.upstreams
            textRole: "text"
            currentIndex: dialog.upstreamIndex
            onActivated: (index) => dialog.upstreamIndex = index
        }
    }

    CheckBox {
        visible: dialog.checkoutVisible
        text: qsTr("Check out the new branch")
        checked: dialog.checkout
        onToggled: dialog.checkout = checked
    }
}
