import QtQuick
import QtQuick.Layouts
import Gittyup

// 'dialog' is the C++ CloneDialog and 'logPanel' shows the clone progress.
DialogPage {
    id: page

    // Keep in sync with CloneDialog::Step.
    readonly property int remoteStep: 0
    readonly property int locationStep: 1
    readonly property int progressStep: 2

    title: dialog.step === remoteStep ? qsTr("Clone a repository")
           : dialog.step === locationStep ? (dialog.init ? qsTr("Initialize a repository")
                                                         : qsTr("Choose a location"))
           : dialog.busy ? qsTr("Cloning...") : qsTr("Clone failed")
    subtitle: dialog.step === remoteStep
              ? (dialog.hosted ? qsTr("Choose the protocol to authenticate with the remote.")
                               : qsTr("Enter the URL of the remote repository or browse for a local directory."))
              : dialog.step === locationStep
                ? qsTr("A new directory is created if it doesn't exist yet.")
                : dialog.busy ? qsTr("The repository opens when the clone finishes.")
                              : qsTr("Go back to try again.")
    contentWidth: 500
    initialFocus: dialog.step === remoteStep ? urlField : nameField
    customAccept: true
    acceptVisible: dialog.step !== progressStep
    acceptEnabled: dialog.canContinue
    acceptText: dialog.step === remoteStep ? qsTr("Next")
                : dialog.init ? qsTr("Initialize") : qsTr("Clone")
    onAcceptRequested: dialog.next()

    extraButtons: PushButton {
        visible: dialog.step === progressStep ? !dialog.busy
                                              : dialog.step === locationStep && !dialog.init
        implicitHeight: 32
        minimumWidth: 88
        text: qsTr("Back")
        icon: "arrow-left"
        onClicked: dialog.back()
    }

    // Progress of the steps.
    Row {
        visible: !dialog.init
        spacing: 6

        Repeater {
            model: [qsTr("Remote"), qsTr("Location"), qsTr("Clone")]

            delegate: Row {
                id: stepItem

                required property int index
                required property string modelData

                readonly property bool done: index < dialog.step
                readonly property bool current: index === dialog.step

                spacing: 6

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 18
                    height: 18
                    radius: 9
                    color: stepItem.done || stepItem.current ? Theme.accent : Theme.field
                    border.color: stepItem.done || stepItem.current ? Theme.accent : Theme.border

                    Text {
                        visible: !stepItem.done
                        anchors.centerIn: parent
                        text: stepItem.index + 1
                        color: stepItem.current ? Theme.accentText : Theme.textMuted
                        font.pixelSize: 10
                        font.bold: true
                    }

                    Icon {
                        visible: stepItem.done
                        anchors.centerIn: parent
                        name: "check"
                        size: 11
                        color: Theme.accentText
                    }
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: stepItem.modelData
                    color: stepItem.current ? Theme.text : Theme.textMuted
                    font.pixelSize: 12
                    font.weight: stepItem.current ? Font.DemiBold : Font.Normal
                }

                Rectangle {
                    visible: stepItem.index < 2
                    anchors.verticalCenter: parent.verticalCenter
                    width: 28
                    height: 1
                    color: Theme.border
                }
            }
        }
    }

    // Remote.
    ColumnLayout {
        Layout.fillWidth: true
        visible: dialog.step === remoteStep
        spacing: 14

        FormField {
            visible: dialog.hosted
            label: qsTr("Protocol")

            ComboBox {
                Layout.fillWidth: true
                model: ["HTTPS", "SSH"]
                currentIndex: dialog.protocol
                onActivated: (index) => dialog.protocol = index
            }
        }

        FormField {
            label: qsTr("URL")
            hint: dialog.hosted ? ""
                                : qsTr("For example https://host/path/repo.git, git@host:path/repo.git or a local directory.")

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                TextField {
                    id: urlField

                    Layout.fillWidth: true
                    readOnly: dialog.hosted
                    placeholderText: qsTr("https://github.com/user/repository.git")
                    text: dialog.url
                    onTextEdited: dialog.url = text
                }

                PushButton {
                    visible: !dialog.hosted
                    implicitHeight: 32
                    icon: "folder"
                    text: qsTr("Browse")
                    onClicked: dialog.browseUrl()
                }
            }
        }
    }

    // Location.
    ColumnLayout {
        Layout.fillWidth: true
        visible: dialog.step === locationStep
        spacing: 14

        FormField {
            label: qsTr("Name")

            TextField {
                id: nameField

                Layout.fillWidth: true
                placeholderText: qsTr("my-repository")
                text: dialog.name
                onTextEdited: dialog.name = text
            }
        }

        FormField {
            label: qsTr("Parent directory")
            error: dialog.directory !== "" && !dialog.canContinue && dialog.name !== ""
                   ? qsTr("This directory doesn't exist.") : ""

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                TextField {
                    Layout.fillWidth: true
                    text: dialog.directory
                    onTextEdited: dialog.directory = text
                }

                PushButton {
                    implicitHeight: 32
                    icon: "folder"
                    text: qsTr("Browse")
                    onClicked: dialog.browseDirectory()
                }
            }
        }

        CheckBox {
            text: qsTr("Create a bare repository")
            checked: dialog.bare
            onToggled: dialog.bare = checked
        }

        Rectangle {
            Layout.fillWidth: true
            visible: dialog.name !== "" && dialog.directory !== ""
            implicitHeight: pathRow.implicitHeight + 16
            radius: 6
            color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.1)
            border.color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.35)

            RowLayout {
                id: pathRow

                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                Icon {
                    name: "repo"
                    size: 14
                    color: Theme.accent
                }

                Text {
                    Layout.fillWidth: true
                    text: dialog.targetPath
                    elide: Text.ElideMiddle
                    color: Theme.text
                    font.pixelSize: 12
                }
            }
        }
    }

    // Progress.
    Rectangle {
        Layout.fillWidth: true
        visible: dialog.step === progressStep
        implicitHeight: 180
        radius: 6
        color: Theme.panel
        border.color: Theme.border
        clip: true

        LogPanel {
            anchors.fill: parent
            anchors.margins: 1
            showHeader: false
        }
    }

    Connections {
        target: dialog

        function onStepChanged() {
            if (dialog.step === page.locationStep)
                nameField.forceActiveFocus()
        }
    }
}
