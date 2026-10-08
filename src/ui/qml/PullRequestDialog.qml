import QtQuick
import QtQuick.Layouts
import Gittyup

// 'dialog' is the C++ PullRequestDialog.
DialogPage {
    title: qsTr("Create a pull request")
    contentWidth: 520
    acceptText: qsTr("Create Pull Request")
    acceptEnabled: dialog.toRepo !== "" && dialog.toBranch !== ""
    customAccept: true
    initialFocus: titleField
    onAcceptRequested: dialog.create()

    FormField {
        label: qsTr("Title")

        TextField {
            id: titleField

            Layout.fillWidth: true
            text: dialog.title
            onTextEdited: dialog.title = text
        }
    }

    FormField {
        label: qsTr("Description")

        TextArea {
            Layout.fillWidth: true
            implicitHeight: 130
            text: dialog.body
            onTextEdited: dialog.body = text
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 12

        FormField {
            Layout.preferredWidth: 1
            label: qsTr("From branch")

            ComboBox {
                Layout.fillWidth: true
                model: dialog.branches
                currentIndex: dialog.branch
                onActivated: (index) => dialog.branch = index
            }
        }

        Icon {
            Layout.topMargin: 20
            name: "arrow-right"
            size: 16
            color: Theme.textMuted
        }

        FormField {
            Layout.preferredWidth: 1
            label: qsTr("Into repository")

            ComboBox {
                Layout.fillWidth: true
                model: dialog.parents
                displayText: dialog.toRepo !== "" ? dialog.toRepo : qsTr("owner/repository")
                onActivated: dialog.chooseParent(currentText)
            }

            TextField {
                Layout.fillWidth: true
                placeholderText: qsTr("branch")
                text: dialog.toBranch
                onTextEdited: dialog.toBranch = text
            }
        }
    }

    CheckBox {
        text: qsTr("Maintainers can modify the pull request")
        checked: dialog.maintainerCanModify
        onToggled: dialog.maintainerCanModify = checked
    }
}
