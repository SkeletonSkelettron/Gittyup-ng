import QtQuick
import QtQuick.Layouts
import Gittyup

// How commits are signed, like GitKraken's GPG preferences: the settings of
// git that 'git commit -S' uses. 'dialog' reads and writes the git
// configuration, the global one when 'global' is set.
SettingSection {
    id: root

    property bool global: false

    readonly property var formats: ["openpgp", "ssh", "x509"]
    // Changes when the configuration is written, to read it again.
    property int revision: 0
    readonly property string format: {
        revision
        const value = dialog.gitConfig("gpg.format").toLowerCase()
        return formats.indexOf(value) >= 0 ? value : "openpgp"
    }

    function isTrue(value) {
        return ["true", "yes", "on", "1"].indexOf(value.toLowerCase()) >= 0
    }

    function write(key, value) {
        dialog.setGitConfig(key, value)
        revision++
    }

    title: qsTr("Commit signing")

    SettingRow {
        label: qsTr("Sign commits")
        hint: root.global ? qsTr("Like commit.gpgsign")
                          : qsTr("Or use the global setting")

        CheckBox {
            visible: root.global
            text: qsTr("Sign every commit")
            checked: root.isTrue(dialog.gitConfig("commit.gpgsign"))
            onToggled: root.write("commit.gpgsign", checked ? "true" : "")
        }

        ComboBox {
            visible: !root.global
            Layout.fillWidth: true
            model: [qsTr("Use the global setting"), qsTr("Always"), qsTr("Never")]
            currentIndex: {
                root.revision
                const value = dialog.gitConfig("commit.gpgsign")
                return value === "" ? 0 : root.isTrue(value) ? 1 : 2
            }
            onActivated: (index) => root.write("commit.gpgsign",
                                               ["", "true", "false"][index])
        }
    }

    SettingRow {
        label: qsTr("Key type")

        ComboBox {
            Layout.fillWidth: true
            model: [qsTr("OpenPGP (gpg)"), qsTr("SSH (ssh-keygen)"), qsTr("X.509 (gpgsm)")]
            currentIndex: root.formats.indexOf(root.format)
            onActivated: (index) => root.write("gpg.format",
                                               index === 0 ? "" : root.formats[index])
        }
    }

    SettingRow {
        label: qsTr("Signing key")
        hint: root.format === "ssh"
              ? qsTr("A public key file, like ~/.ssh/id_ed25519.pub, or key:: and a key of the SSH agent")
              : qsTr("The key id, or empty for the key of your email")

        TextField {
            Layout.fillWidth: true
            text: {
                root.revision
                return dialog.gitConfig("user.signingkey")
            }
            placeholderText: root.format === "ssh" ? "~/.ssh/id_ed25519.pub" : ""
            onTextEdited: dialog.setGitConfig("user.signingkey", text.trim())
        }
    }
}
