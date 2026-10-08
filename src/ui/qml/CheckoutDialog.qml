import QtQuick
import QtQuick.Layouts
import Gittyup

// 'dialog' is the C++ CheckoutDialog.
DialogPage {
    initialFocus: refCombo
    title: qsTr("Check out")
    subtitle: qsTr("Switch the working directory to a branch, a tag or a remote branch.")
    acceptText: qsTr("Checkout")
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

    CheckBox {
        enabled: dialog.detachEnabled
        text: qsTr("Detach HEAD")
        checked: dialog.detachChecked
        onToggled: dialog.setDetach(checked)
    }
}
