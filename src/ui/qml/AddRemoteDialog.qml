import QtQuick
import QtQuick.Layouts
import Gittyup

// 'dialog' is the C++ AddRemoteDialog.
DialogPage {
    initialFocus: dialog.name === "" ? nameField : urlField
    title: qsTr("Add a remote")
    acceptText: qsTr("Add Remote")
    acceptEnabled: dialog.acceptable

    FormField {
        label: qsTr("Name")

        TextField {
            id: nameField

            Layout.fillWidth: true
            placeholderText: qsTr("origin")
            text: dialog.name
            onTextEdited: dialog.name = text
        }
    }

    FormField {
        label: qsTr("URL")

        TextField {
            id: urlField

            Layout.fillWidth: true
            placeholderText: qsTr("https://example.com/user/repository.git")
            text: dialog.url
            onTextEdited: dialog.url = text
        }
    }
}
