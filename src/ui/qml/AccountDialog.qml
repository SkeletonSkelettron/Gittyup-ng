import QtQuick
import QtQuick.Layouts
import Gittyup

// 'dialog' is the C++ AccountDialog.
DialogPage {
    initialFocus: usernameField
    title: qsTr("Connect a hosting service")
    subtitle: qsTr("Browse and clone your remote repositories from %1.").arg(Qt.application.name)
    acceptText: dialog.busy ? qsTr("Connecting...") : qsTr("Connect")
    acceptEnabled: dialog.acceptable

    FormField {
        label: qsTr("Host")

        ComboBox {
            Layout.fillWidth: true
            enabled: !dialog.busy
            model: dialog.hosts
            textRole: "text"
            currentIndex: dialog.hostIndex
            onActivated: (index) => dialog.hostIndex = index
        }
    }

    FormField {
        label: qsTr("Username")

        TextField {
            id: usernameField

            Layout.fillWidth: true
            enabled: !dialog.busy
            text: dialog.username
            onTextEdited: dialog.username = text
        }
    }

    FormField {
        label: qsTr("Password or access token")

        TextField {
            Layout.fillWidth: true
            enabled: !dialog.busy
            echoMode: TextInput.Password
            text: dialog.password
            onTextEdited: dialog.password = text
        }
    }

    Text {
        Layout.fillWidth: true
        visible: dialog.helpText !== ""
        text: dialog.helpText
        textFormat: Text.StyledText
        wrapMode: Text.Wrap
        color: Theme.textMuted
        linkColor: Theme.accent
        font.pixelSize: 12
        onLinkActivated: (link) => Qt.openUrlExternally(link)

        HoverHandler {
            cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
        }
    }

    FormField {
        label: qsTr("URL")

        TextField {
            Layout.fillWidth: true
            enabled: !dialog.busy
            text: dialog.url
            onTextEdited: dialog.url = text
        }
    }
}
