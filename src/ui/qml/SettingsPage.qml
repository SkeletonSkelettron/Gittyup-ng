import QtQuick
import QtQuick.Controls.Basic as Controls
import QtQuick.Layouts
import Gittyup

// The application settings. 'dialog' is the C++ SettingsDialog.
PreferencesPage {
    id: root

    // Keep in sync with SettingsDialog::Index.
    sections: [
        { title: qsTr("General"), icon: "sliders",
          description: qsTr("Your identity, automatic actions and credentials. Repositories can override git settings.") },
        { title: qsTr("Diff"), icon: "view-double",
          description: qsTr("How changes are compared and shown.") },
        { title: qsTr("Tools"), icon: "wrench",
          description: qsTr("External programs that %1 starts.").arg(Qt.application.name) },
        { title: qsTr("Appearance"), icon: "palette",
          description: qsTr("The theme, the window and the prompts.") },
        { title: qsTr("Editor"), icon: "code",
          description: qsTr("The font of code in diffs and files, and how the editor indents.") },
        { title: qsTr("Updates"), icon: "download",
          description: qsTr("Keep %1 up to date.").arg(Qt.application.name) },
        { title: qsTr("Plugins"), icon: "plug",
          description: qsTr("Lua plugins that check your changes.") },
        { title: qsTr("SSH"), icon: "key",
          description: qsTr("The SSH configuration used for remotes.") },
        { title: qsTr("Hotkeys"), icon: "keyboard",
          description: qsTr("The keyboard shortcuts of the menu actions.") },
        { title: qsTr("Terminal"), icon: "terminal",
          description: qsTr("Start %1 from a terminal.").arg(Qt.application.name) }
    ]

    title: qsTr("Settings")
    pages: [general, diff, tools, appearance, editor, updates, plugins, ssh, hotkeys, terminal]
    current: dialog.section
    hiddenSections: dialog.terminalVisible ? [] : [9]
    sideButtonText: qsTr("Edit Git Config File")
    sideButtonIcon: "file"
    sideButtonTip: qsTr("Open the global git configuration in the editor")
    onSectionSelected: (index) => dialog.section = index
    onSideButtonClicked: dialog.editConfigFile()
    onCloseRequested: dialog.close()

    // Check boxes for settings.
    component SettingCheck: CheckBox {
        property string setting

        checked: dialog.settingBool(setting)
        onToggled: dialog.setSetting(setting, checked)
    }

    component PromptCheck: CheckBox {
        property string kind

        text: dialog.promptText(kind)
        checked: dialog.prompt(kind)
        onToggled: dialog.setPrompt(kind, checked)
    }

    component SettingField: TextField {
        property string setting

        Layout.fillWidth: true
        text: dialog.settingString(setting)
        onTextEdited: dialog.setSetting(setting, text)
    }

    Component {
        id: general

        ColumnLayout {
            spacing: 20

            SettingSection {
                title: qsTr("Identity")

                SettingRow {
                    label: qsTr("Name")
                    hint: qsTr("The author of your commits")

                    TextField {
                        Layout.fillWidth: true
                        text: dialog.gitConfig("user.name")
                        onTextEdited: dialog.setGitConfig("user.name", text)
                    }
                }

                SettingRow {
                    label: qsTr("Email")

                    TextField {
                        Layout.fillWidth: true
                        text: dialog.gitConfig("user.email")
                        onTextEdited: dialog.setGitConfig("user.email", text)
                    }
                }
            }

            SigningSection {
                global: true
            }

            SettingSection {
                title: qsTr("Automatic actions")

                SettingRow {
                    label: qsTr("Fetch")

                    RowLayout {
                        spacing: 10

                        SettingCheck {
                            id: fetchCheck

                            setting: "FetchAutomatically"
                            text: qsTr("Fetch every")
                        }

                        SpinBox {
                            enabled: fetchCheck.checked
                            from: 1
                            to: 1440
                            value: dialog.settingInt("AutomaticFetchPeriodInMinutes")
                            onValueModified: dialog.setSetting("AutomaticFetchPeriodInMinutes", value)
                        }

                        Text {
                            text: qsTr("minutes")
                            color: Theme.textMuted
                            font.pixelSize: 13
                        }
                    }
                }

                SettingRow {
                    label: qsTr("After actions")

                    SettingCheck {
                        setting: "PushAfterEachCommit"
                        text: qsTr("Push after each commit")
                    }

                    SettingCheck {
                        setting: "UpdateSubmodulesAfterPullAndClone"
                        text: qsTr("Update submodules after pull and clone")
                    }

                    SettingCheck {
                        setting: "PruneAfterFetch"
                        text: qsTr("Prune when fetching")
                    }
                }
            }

            SettingSection {
                title: qsTr("Language")

                SettingRow {
                    label: qsTr("Language")
                    hint: qsTr("Takes effect after a restart")

                    SettingCheck {
                        id: noTranslation

                        setting: "DontTranslate"
                        text: qsTr("Don't translate, use English")
                    }

                    ComboBox {
                        Layout.fillWidth: true
                        enabled: !noTranslation.checked
                        model: dialog.languages
                        textRole: "text"
                        currentIndex: dialog.language()
                        onActivated: (index) => dialog.setLanguage(index)
                    }
                }
            }

            SettingSection {
                title: qsTr("Credentials")

                SettingRow {
                    label: qsTr("Secure storage")

                    CheckBox {
                        id: storeCheck

                        text: qsTr("Store credentials in secure storage")
                        checked: dialog.storeCredentials()
                        onToggled: dialog.setStoreCredentials(checked, storeCombo.currentText)
                    }

                    ComboBox {
                        id: storeCombo

                        Layout.fillWidth: true
                        enabled: storeCheck.checked
                        model: dialog.credentialStores
                        currentIndex: Math.max(0, dialog.credentialStores.indexOf(dialog.credentialStore()))
                        onActivated: dialog.setStoreCredentials(true, currentText)
                    }

                    Text {
                        Layout.fillWidth: true
                        text: dialog.credentialStoresInfo
                        textFormat: Text.StyledText
                        wrapMode: Text.Wrap
                        color: Theme.textMuted
                        font.pixelSize: 12
                    }

                    Text {
                        text: qsTr("<a href=\"privacy\">View the privacy policy</a>")
                        textFormat: Text.StyledText
                        linkColor: Theme.accent
                        font.pixelSize: 12
                        onLinkActivated: dialog.showPrivacyPolicy()

                        HoverHandler {
                            cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                        }
                    }
                }
            }

            SettingSection {
                visible: dialog.singleInstanceVisible
                title: qsTr("Application")

                SettingRow {
                    label: qsTr("Instances")

                    SettingCheck {
                        setting: "AllowSingleInstanceOnly"
                        text: qsTr("Only allow a single running instance")
                    }
                }
            }
        }
    }

    Component {
        id: diff

        ColumnLayout {
            spacing: 20

            SettingSection {
                title: qsTr("Content")

                SettingRow {
                    label: qsTr("Context lines")
                    hint: qsTr("Unchanged lines around each change")

                    SpinBox {
                        from: 0
                        to: 100
                        value: dialog.diffContext()
                        onValueModified: dialog.setDiffContext(value)
                    }
                }

                SettingRow {
                    label: qsTr("Whitespace")

                    CheckBox {
                        text: qsTr("Ignore whitespace (-w)")
                        checked: dialog.ignoreWhitespace()
                        onToggled: dialog.setIgnoreWhitespace(checked)
                    }
                }

                SettingRow {
                    label: qsTr("Character encoding")

                    ComboBox {
                        Layout.fillWidth: true
                        model: dialog.encodings
                        currentIndex: dialog.encoding()
                        onActivated: (index) => dialog.setEncoding(index)
                    }
                }
            }
        }
    }

    component ToolRow: SettingRow {
        id: toolRow

        property string type

        RowLayout {
            spacing: 8

            ComboBox {
                id: toolCombo

                Layout.fillWidth: true
                model: dialog.tools(toolRow.type)
                currentIndex: model.indexOf(dialog.tool(toolRow.type))
                displayText: currentIndex < 0 ? qsTr("None") : currentText
                onActivated: dialog.setTool(toolRow.type, currentText)

                Connections {
                    target: dialog

                    function onConfigChanged() {
                        toolCombo.model = dialog.tools(toolRow.type)
                        toolCombo.currentIndex = toolCombo.model.indexOf(dialog.tool(toolRow.type))
                    }
                }
            }

            PushButton {
                implicitHeight: 32
                text: qsTr("Configure")
                onClicked: dialog.configureTools(toolRow.type)
            }
        }
    }

    Component {
        id: tools

        ColumnLayout {
            spacing: 20

            SettingSection {
                title: qsTr("Git tools")

                SettingRow {
                    label: qsTr("External editor")
                    hint: qsTr("Opens files for editing")

                    TextField {
                        Layout.fillWidth: true
                        placeholderText: qsTr("code --wait")
                        text: dialog.gitConfig("gui.editor")
                        onTextEdited: dialog.setGitConfig("gui.editor", text)
                    }
                }

                ToolRow {
                    label: qsTr("Diff tool")
                    type: "diff"
                }

                ToolRow {
                    label: qsTr("Merge tool")
                    type: "merge"
                }

                SettingRow {
                    label: qsTr("Backup files")

                    CheckBox {
                        text: qsTr("Keep backups of merged files (.orig)")
                        checked: dialog.gitConfigBool("mergetool.keepBackup")
                        onToggled: dialog.setGitConfig("mergetool.keepBackup", checked)
                    }
                }
            }

            SettingSection {
                title: qsTr("System")

                SettingRow {
                    label: qsTr("Terminal")
                    hint: qsTr("The command that opens a terminal")

                    SettingField {
                        setting: "TerminalCommand"
                    }
                }

                SettingRow {
                    label: qsTr("File manager")
                    hint: qsTr("%1 is replaced by the repository path").arg("\"%1\"")

                    SettingField {
                        setting: "FilemanagerCommand"
                    }
                }
            }
        }
    }

    Component {
        id: appearance

        ColumnLayout {
            spacing: 20

            SettingSection {
                title: qsTr("Theme")

                SettingRow {
                    label: qsTr("Color theme")
                    hint: qsTr("Takes effect after a restart")

                    RowLayout {
                        spacing: 8

                        ComboBox {
                            Layout.fillWidth: true
                            model: dialog.themes
                            currentIndex: dialog.themes.indexOf(dialog.theme)
                            onActivated: dialog.setTheme(currentText)
                        }

                        PushButton {
                            implicitHeight: 32
                            enabled: dialog.themeEditable
                            icon: "pencil"
                            text: qsTr("Edit")
                            tip: qsTr("Only your own themes can be edited")
                            onClicked: dialog.editTheme()
                        }
                    }

                    RowLayout {
                        spacing: 8

                        TextField {
                            id: themeName

                            Layout.fillWidth: true
                            placeholderText: qsTr("Name of a new theme")
                        }

                        PushButton {
                            implicitHeight: 32
                            enabled: themeName.text !== ""
                            icon: "plus"
                            text: qsTr("Create Theme")
                            onClicked: dialog.addTheme(themeName.text)
                        }
                    }
                }
            }

            SettingSection {
                title: qsTr("Window")

                SettingRow {
                    label: qsTr("Window")

                    SettingCheck {
                        setting: "ShowFullRepoPath"
                        text: qsTr("Show the full repository path in the title")
                    }

                    SettingCheck {
                        setting: "ShowMaximized"
                        text: qsTr("Maximize windows when they open")
                    }

                    SettingCheck {
                        setting: "HideMenuBar"
                        text: qsTr("Hide the menu bar")
                    }

                    SettingCheck {
                        setting: "ShowAvatars"
                        text: qsTr("Show avatars")
                    }
                }

                SettingRow {
                    label: qsTr("Tabs")

                    SettingCheck {
                        setting: "OpenAllReposInTabs"
                        text: qsTr("Open all repositories in tabs")
                    }

                    SettingCheck {
                        setting: "OpenSubmodulesInTabs"
                        text: qsTr("Open submodules in tabs")
                    }

                    SettingCheck {
                        setting: "AutoHideRepoSiderbar"
                        text: qsTr("Hide the repository sidebar after opening a repository")
                    }
                }

                SettingRow {
                    label: qsTr("Activity log")

                    SettingCheck {
                        setting: "HideLogAutomatically"
                        text: qsTr("Hide the log automatically")
                    }
                }
            }

            SettingSection {
                title: qsTr("Prompts")

                SettingRow {
                    label: qsTr("Ask before")

                    PromptCheck { kind: "Merge" }
                    PromptCheck { kind: "Revert" }
                    PromptCheck { kind: "CherryPick" }
                    PromptCheck { kind: "Stash" }
                    PromptCheck { kind: "Directories" }
                    PromptCheck { kind: "LargeFiles" }
                }
            }
        }
    }

    Component {
        id: editor

        ColumnLayout {
            spacing: 20

            SettingSection {
                title: qsTr("Font")

                SettingRow {
                    label: qsTr("Font")

                    ComboBox {
                        Layout.fillWidth: true
                        model: dialog.fonts
                        currentIndex: dialog.fonts.indexOf(dialog.settingString("FontFamily"))
                        onActivated: dialog.setSetting("FontFamily", currentText)
                    }
                }

                SettingRow {
                    label: qsTr("Size")

                    SpinBox {
                        from: 4
                        to: 72
                        value: dialog.settingInt("FontSize")
                        onValueModified: dialog.setSetting("FontSize", value)
                    }
                }
            }

            SettingSection {
                title: qsTr("Indentation")

                SettingRow {
                    label: qsTr("Indent using")

                    SegmentedControl {
                        model: [qsTr("Tabs"), qsTr("Spaces")]
                        currentIndex: dialog.settingBool("UseTabsForIndent") ? 0 : 1
                        onActivated: (index) => dialog.setSetting("UseTabsForIndent", index === 0)
                    }
                }

                SettingRow {
                    label: qsTr("Indent width")

                    SpinBox {
                        from: 1
                        to: 16
                        value: dialog.settingInt("IndentWidth")
                        onValueModified: dialog.setSetting("IndentWidth", value)
                    }
                }

                SettingRow {
                    label: qsTr("Tab width")

                    SpinBox {
                        from: 1
                        to: 16
                        value: dialog.settingInt("TabWidth")
                        onValueModified: dialog.setSetting("TabWidth", value)
                    }
                }
            }

            SettingSection {
                title: qsTr("Display")

                SettingRow {
                    label: qsTr("Editor")

                    SettingCheck {
                        setting: "ShowWhitespaceInEditor"
                        text: qsTr("Show whitespace")
                    }

                    SettingCheck {
                        setting: "ShowHeatmapInBlameMargin"
                        text: qsTr("Show a heat map in the blame margin")
                    }
                }

                SettingRow {
                    label: qsTr("Long lines")

                    CheckBox {
                        text: qsTr("Wrap lines in the editor")
                        checked: dialog.wrapLines()
                        onToggled: dialog.setWrapLines(checked)
                    }
                }
            }
        }
    }

    Component {
        id: updates

        ColumnLayout {
            spacing: 20

            SettingSection {
                title: qsTr("Software update")

                SettingRow {
                    label: qsTr("Updates")

                    SettingCheck {
                        setting: "CheckForUpdatesAutomatically"
                        text: qsTr("Check for updates automatically")
                    }

                    SettingCheck {
                        visible: dialog.updateDownloadVisible
                        setting: "InstallUpdatesAutomatically"
                        text: qsTr("Download and install updates automatically")
                    }

                    PushButton {
                        implicitHeight: 32
                        icon: "refresh"
                        text: qsTr("Check Now")
                        onClicked: dialog.checkForUpdates()
                    }
                }
            }
        }
    }

    Component {
        id: plugins

        ColumnLayout {
            spacing: 20

            SettingSection {
                title: qsTr("Plugins")

                SettingRow {
                    label: qsTr("Plugins")
                    hint: qsTr("Enable plugins and set their options")

                    PushButton {
                        implicitHeight: 32
                        icon: "plug"
                        text: qsTr("Configure Plugins...")
                        onClicked: dialog.configurePlugins()
                    }
                }
            }
        }
    }

    Component {
        id: ssh

        ColumnLayout {
            spacing: 20

            SettingSection {
                title: qsTr("SSH")

                SettingRow {
                    label: qsTr("Config file")

                    SettingField {
                        setting: "SshConfigFilePath"
                        placeholderText: "~/.ssh/config"
                    }
                }

                SettingRow {
                    label: qsTr("Key file")
                    hint: qsTr("The default or fallback key")

                    SettingField {
                        setting: "SshKeyFilePath"
                        placeholderText: "~/.ssh/id_ed25519"
                    }
                }
            }
        }
    }

    Component {
        id: hotkeys

        ColumnLayout {
            id: hotkeyPage

            // The index of the hotkey that records keys, or -1.
            property int recording: -1
            property string recorded
            property string conflicts

            spacing: 12

            TextField {
                id: hotkeyFilter

                Layout.fillWidth: true
                placeholderText: qsTr("Filter actions")
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("Click a shortcut and press the new keys. Backspace removes the shortcut and Escape cancels.")
                wrapMode: Text.Wrap
                color: Theme.textMuted
                font.pixelSize: 12
            }

            Repeater {
                model: dialog.hotkeys

                delegate: ColumnLayout {
                    id: hotkey

                    required property var modelData
                    required property int index

                    readonly property bool matches: {
                        const filter = hotkeyFilter.text.toLowerCase()
                        return filter === ""
                               || modelData.label.toLowerCase().indexOf(filter) >= 0
                               || modelData.group.toLowerCase().indexOf(filter) >= 0
                               || modelData.keys.toLowerCase().indexOf(filter) >= 0
                    }
                    readonly property bool firstOfGroup: {
                        const all = dialog.hotkeys
                        for (let i = index - 1; i >= 0; --i) {
                            const previous = all[i]
                            if (previous.group !== modelData.group)
                                return true
                            const filter = hotkeyFilter.text.toLowerCase()
                            if (filter === "" || previous.label.toLowerCase().indexOf(filter) >= 0
                                    || previous.group.toLowerCase().indexOf(filter) >= 0
                                    || previous.keys.toLowerCase().indexOf(filter) >= 0)
                                return false
                        }
                        return true
                    }
                    readonly property bool isRecording: hotkeyPage.recording === modelData.index

                    Layout.fillWidth: true
                    visible: matches
                    spacing: 4

                    Text {
                        visible: hotkey.firstOfGroup && hotkey.modelData.group !== ""
                        Layout.topMargin: 10
                        text: hotkey.modelData.group
                        color: Theme.textMuted
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                        font.letterSpacing: 0.8
                        font.capitalization: Font.AllUppercase
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Text {
                            Layout.fillWidth: true
                            text: hotkey.modelData.label
                            elide: Text.ElideRight
                            color: Theme.text
                            font.pixelSize: 13
                        }

                        Text {
                            visible: hotkey.isRecording && hotkeyPage.conflicts !== ""
                            text: qsTr("Also used by %1").arg(hotkeyPage.conflicts)
                            color: Theme.modified
                            font.pixelSize: 11
                        }

                        // The shortcut. Click to record new keys.
                        Rectangle {
                            id: keyChip

                            implicitWidth: Math.max(96, keyLabel.implicitWidth + 20)
                            implicitHeight: 28
                            radius: 6
                            color: hotkey.isRecording ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.16)
                                                      : chipMouse.containsMouse ? Theme.hover : Theme.field
                            border.width: hotkey.isRecording ? 2 : 1
                            border.color: hotkey.isRecording ? Theme.accent : Theme.border
                            focus: hotkey.isRecording

                            Text {
                                id: keyLabel

                                anchors.centerIn: parent
                                text: hotkey.isRecording
                                      ? (hotkeyPage.recorded !== "" ? hotkeyPage.recorded : qsTr("Press keys..."))
                                      : (hotkey.modelData.keys !== "" ? hotkey.modelData.keys : qsTr("None"))
                                color: hotkey.isRecording ? Theme.accent
                                       : hotkey.modelData.keys !== "" ? Theme.text : Theme.textMuted
                                font.family: Theme.monoFont
                                font.pixelSize: 12
                            }

                            MouseArea {
                                id: chipMouse

                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    hotkeyPage.recording = hotkey.modelData.index
                                    hotkeyPage.recorded = ""
                                    hotkeyPage.conflicts = ""
                                    keyChip.forceActiveFocus()
                                }
                            }

                            // Take every key, including the shortcuts of the menus.
                            Keys.onShortcutOverride: (event) => event.accepted = hotkey.isRecording
                            Keys.onPressed: (event) => {
                                if (!hotkey.isRecording)
                                    return
                                event.accepted = true
                                const modifiers = event.modifiers & (Qt.ShiftModifier | Qt.ControlModifier
                                                                     | Qt.AltModifier | Qt.MetaModifier)
                                if (event.key === Qt.Key_Escape && !modifiers) {
                                    hotkeyPage.recording = -1
                                    return
                                }
                                if ((event.key === Qt.Key_Backspace || event.key === Qt.Key_Delete) && !modifiers) {
                                    dialog.clearHotkey(hotkey.modelData.index)
                                    hotkeyPage.recording = -1
                                    return
                                }

                                hotkeyPage.recorded = dialog.keyText(event.key, modifiers)
                                const modifierKeys = [Qt.Key_Shift, Qt.Key_Control, Qt.Key_Meta,
                                                      Qt.Key_Alt, Qt.Key_AltGr]
                                if (modifierKeys.indexOf(event.key) >= 0)
                                    return

                                hotkeyPage.conflicts = dialog.hotkeyConflicts(hotkey.modelData.index,
                                                                              event.key, modifiers)
                                dialog.setHotkey(hotkey.modelData.index, event.key, modifiers)
                                if (hotkeyPage.conflicts === "")
                                    hotkeyPage.recording = -1
                            }
                            onActiveFocusChanged: {
                                if (!activeFocus && hotkey.isRecording)
                                    hotkeyPage.recording = -1
                            }
                        }

                        ActionButton {
                            compact: true
                            implicitWidth: 28
                            implicitHeight: 28
                            opacity: hotkey.modelData.custom ? 1 : 0
                            enabled: hotkey.modelData.custom
                            icon: "refresh"
                            tip: qsTr("Reset to the default")
                            onClicked: dialog.resetHotkey(hotkey.modelData.index)
                        }
                    }
                }
            }
        }
    }

    Component {
        id: terminal

        ColumnLayout {
            spacing: 20

            SettingSection {
                title: qsTr("Command line tool")

                SettingRow {
                    label: qsTr("Name")

                    SettingField {
                        setting: "TerminalName"
                        onTextEdited: dialog.terminalChanged()
                    }
                }

                SettingRow {
                    label: qsTr("Location")

                    SettingField {
                        setting: "TerminalPath"
                        onTextEdited: dialog.terminalChanged()
                    }

                    PushButton {
                        implicitHeight: 32
                        enabled: dialog.terminalInstallEnabled
                        text: dialog.terminalInstallText
                        onClicked: dialog.toggleTerminalInstall()
                    }
                }
            }
        }
    }
}
