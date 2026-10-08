import QtQuick
import QtQuick.Layouts
import Gittyup

// 'dialog' is the C++ RemoteDialog.
DialogPage {
    // Keep in sync with RemoteDialog::Kind.
    readonly property int fetch: 0
    readonly property int pull: 1
    readonly property int push: 2

    title: dialog.kind === fetch ? qsTr("Fetch from a remote")
           : dialog.kind === pull ? qsTr("Pull from a remote") : qsTr("Push to a remote")
    acceptText: dialog.kind === fetch ? qsTr("Fetch")
                : dialog.kind === pull ? qsTr("Pull") : qsTr("Push")
    acceptEnabled: dialog.remote.trim() !== ""
    initialFocus: remoteField

    FormField {
        label: qsTr("Remote")
        hint: qsTr("The name of a remote or a URL.")

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            TextField {
                id: remoteField

                Layout.fillWidth: true
                text: dialog.remote
                onTextEdited: dialog.remote = text
            }

            ActionButton {
                visible: dialog.hasRemotes
                compact: true
                hasMenu: true
                menuOnly: true
                implicitWidth: 32
                implicitHeight: 32
                icon: "chevron-down"
                tip: qsTr("Choose a remote")
                onMenuRequested: (x, y) => dialog.showRemoteMenu(x, y)
            }
        }
    }

    FormField {
        visible: dialog.kind === push
        label: qsTr("Reference")

        ComboBox {
            Layout.fillWidth: true
            model: dialog.refs
            textRole: "text"
            currentIndex: dialog.refIndex
            onActivated: (index) => dialog.refIndex = index
        }
    }

    FormField {
        visible: dialog.kind === push
        label: qsTr("Remote reference")

        TextField {
            Layout.fillWidth: true
            placeholderText: qsTr("refs/heads/main")
            text: dialog.remoteRef
            onTextEdited: dialog.remoteRef = text
        }
    }

    FormField {
        visible: dialog.kind === pull
        label: qsTr("Action")

        ComboBox {
            Layout.fillWidth: true
            model: dialog.actions
            currentIndex: dialog.action
            onActivated: (index) => dialog.action = index
        }
    }

    ColumnLayout {
        spacing: 10

        CheckBox {
            text: dialog.kind === push ? qsTr("Push all tags") : qsTr("Update existing tags")
            checked: dialog.tags
            onToggled: dialog.tags = checked
        }

        CheckBox {
            visible: dialog.kind !== push
            text: qsTr("Prune references that were deleted on the remote")
            checked: dialog.prune
            onToggled: dialog.prune = checked
        }

        CheckBox {
            visible: dialog.kind === push
            text: qsTr("Set upstream")
            checked: dialog.setUpstream
            onToggled: dialog.setUpstream = checked
        }

        CheckBox {
            visible: dialog.kind === push
            text: qsTr("Force")
            checked: dialog.force
            onToggled: dialog.force = checked
        }
    }
}
