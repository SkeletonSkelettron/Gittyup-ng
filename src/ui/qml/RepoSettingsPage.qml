import QtQuick
import QtQuick.Layouts
import Gittyup

// The settings of a repository. 'dialog' is the C++ ConfigDialog.
PreferencesPage {
    id: root

    // Keep in sync with ConfigDialog::Index.
    sections: [
        { title: qsTr("General"), icon: "sliders",
          description: qsTr("Settings of this repository that override the application settings.") },
        { title: qsTr("Diff"), icon: "view-double",
          description: qsTr("How changes of this repository are compared.") },
        { title: qsTr("Remotes"), icon: "cloud",
          description: qsTr("The remote repositories to fetch from and push to.") },
        { title: qsTr("Branches"), icon: "branch",
          description: qsTr("The local branches and the branches they track.") },
        { title: qsTr("Submodules"), icon: "repo",
          description: qsTr("Repositories that are nested in this one.") },
        { title: qsTr("Search"), icon: "search",
          description: qsTr("The index that makes searching commits fast.") },
        { title: qsTr("Plugins"), icon: "plug",
          description: qsTr("Lua plugins that check the changes of this repository.") },
        { title: qsTr("LFS"), icon: "download",
          description: qsTr("Git Large File Storage keeps large files outside the repository.") }
    ]

    title: dialog.repoName
    pages: [general, diff, remotes, branches, submodules, search, plugins, lfs]
    current: dialog.section
    sideButtonText: qsTr("Edit Config File")
    sideButtonIcon: "file"
    sideButtonTip: qsTr("Open the configuration of this repository in the editor")
    onSectionSelected: (index) => dialog.section = index
    onSideButtonClicked: dialog.editConfigFile()
    onCloseRequested: dialog.close()

    // A bordered list with an empty state.
    component ListFrame: Rectangle {
        id: frame

        property string emptyText
        property int count: 0

        default property alias content: rows.data

        Layout.fillWidth: true
        implicitHeight: count ? rows.implicitHeight + 8 : 64
        radius: 8
        color: Theme.base
        border.color: Theme.border

        Text {
            visible: frame.count === 0
            anchors.centerIn: parent
            text: frame.emptyText
            color: Theme.textMuted
            font.pixelSize: 12
        }

        ColumnLayout {
            id: rows

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 4
            spacing: 2
        }
    }

    Component {
        id: general

        ColumnLayout {
            spacing: 20

            SettingSection {
                title: qsTr("Identity")

                SettingRow {
                    label: qsTr("Name")
                    hint: qsTr("Leave empty to use the global name")

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

            SigningSection {}

            SettingSection {
                title: qsTr("Automatic actions")

                SettingRow {
                    label: qsTr("Fetch")

                    RowLayout {
                        spacing: 10

                        CheckBox {
                            text: qsTr("Fetch every")
                            checked: dialog.fetchEnabled
                            onToggled: dialog.fetchEnabled = checked
                        }

                        SpinBox {
                            enabled: dialog.fetchEnabled
                            from: 1
                            to: 1440
                            value: dialog.fetchMinutes
                            onValueModified: dialog.fetchMinutes = value
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

                    CheckBox {
                        text: qsTr("Push after each commit")
                        checked: dialog.pushAfterCommit
                        onToggled: dialog.pushAfterCommit = checked
                    }

                    CheckBox {
                        text: qsTr("Update submodules after pull and clone")
                        checked: dialog.updateSubmodules
                        onToggled: dialog.updateSubmodules = checked
                    }

                    CheckBox {
                        text: qsTr("Prune when fetching")
                        checked: dialog.pruneAfterFetch
                        onToggled: dialog.pruneAfterFetch = checked
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
                    label: qsTr("Character encoding")

                    ComboBox {
                        Layout.fillWidth: true
                        model: dialog.encodings()
                        currentIndex: dialog.encoding()
                        onActivated: (index) => dialog.setEncoding(index)
                    }
                }
            }
        }
    }

    Component {
        id: remotes

        ColumnLayout {
            spacing: 12

            ListFrame {
                count: dialog.remotes.length
                emptyText: qsTr("This repository has no remotes.")

                Repeater {
                    model: dialog.remotes

                    delegate: RowLayout {
                        id: remote

                        required property int index
                        required property var modelData

                        Layout.fillWidth: true
                        Layout.margins: 4
                        spacing: 8

                        Icon {
                            name: "cloud"
                            size: 16
                            color: Theme.accent
                        }

                        TextField {
                            Layout.preferredWidth: 130
                            text: remote.modelData.name
                            onEditingFinished: dialog.renameRemote(remote.index, text)
                        }

                        TextField {
                            Layout.fillWidth: true
                            text: remote.modelData.url
                            onEditingFinished: dialog.setRemoteUrl(remote.index, text)
                        }

                        ActionButton {
                            compact: true
                            implicitWidth: 30
                            implicitHeight: 30
                            icon: "close"
                            foregroundOverride: Theme.deleted
                            tip: qsTr("Delete the remote")
                            onClicked: dialog.deleteRemote(remote.index)
                        }
                    }
                }
            }

            PushButton {
                implicitHeight: 32
                icon: "plus"
                text: qsTr("Add Remote")
                onClicked: dialog.addRemoteWithName("")
            }
        }
    }

    Component {
        id: branches

        ColumnLayout {
            spacing: 12

            ListFrame {
                count: dialog.branches.length
                emptyText: qsTr("This repository has no branches yet.")

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 36
                    Layout.rightMargin: 42
                    Layout.topMargin: 4
                    visible: dialog.branches.length > 0
                    spacing: 8

                    Text {
                        Layout.preferredWidth: 170
                        text: qsTr("NAME")
                        color: Theme.textMuted
                        font.pixelSize: 10
                        font.weight: Font.DemiBold
                        font.letterSpacing: 0.6
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("UPSTREAM")
                        color: Theme.textMuted
                        font.pixelSize: 10
                        font.weight: Font.DemiBold
                        font.letterSpacing: 0.6
                    }

                    Text {
                        text: qsTr("REBASE")
                        color: Theme.textMuted
                        font.pixelSize: 10
                        font.weight: Font.DemiBold
                        font.letterSpacing: 0.6
                    }
                }

                Repeater {
                    model: dialog.branches

                    delegate: RowLayout {
                        id: branch

                        required property int index
                        required property var modelData

                        Layout.fillWidth: true
                        Layout.margins: 4
                        spacing: 8

                        Icon {
                            name: branch.modelData.head ? "check" : "branch"
                            size: 16
                            color: branch.modelData.head ? Theme.accent : Theme.textMuted
                        }

                        TextField {
                            id: branchName

                            Layout.preferredWidth: 170
                            enabled: !branch.modelData.head
                            text: branch.modelData.name
                            font.bold: branch.modelData.head
                            onEditingFinished: {
                                if (!dialog.renameBranch(branch.index, text))
                                    text = branch.modelData.name
                            }

                            Connections {
                                target: dialog

                                function onEditBranchRequested(name) {
                                    if (name === branch.modelData.name) {
                                        branchName.forceActiveFocus()
                                        branchName.selectAll()
                                    }
                                }
                            }
                        }

                        ComboBox {
                            Layout.fillWidth: true
                            model: dialog.upstreams
                            currentIndex: branch.modelData.upstream
                            onActivated: (index) => dialog.setBranchUpstream(branch.index, index)
                        }

                        CheckBox {
                            checked: branch.modelData.rebase
                            onToggled: dialog.setBranchRebase(branch.index, checked)
                        }

                        ActionButton {
                            compact: true
                            implicitWidth: 30
                            implicitHeight: 30
                            enabled: !branch.modelData.head
                            icon: "close"
                            foregroundOverride: enabled ? Theme.deleted : "transparent"
                            tip: qsTr("Delete the branch")
                            onClicked: dialog.deleteBranch(branch.index)
                        }
                    }
                }
            }

            PushButton {
                implicitHeight: 32
                icon: "plus"
                text: qsTr("New Branch")
                onClicked: dialog.newBranch()
            }
        }
    }

    Component {
        id: submodules

        ColumnLayout {
            spacing: 12

            ListFrame {
                count: dialog.submodules.length
                emptyText: qsTr("This repository has no submodules.")

                Repeater {
                    model: dialog.submodules

                    delegate: ColumnLayout {
                        id: submodule

                        required property int index
                        required property var modelData

                        Layout.fillWidth: true
                        Layout.margins: 6
                        spacing: 6

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Icon {
                                name: "repo"
                                size: 16
                                color: Theme.accent
                            }

                            Text {
                                Layout.fillWidth: true
                                text: submodule.modelData.name
                                elide: Text.ElideMiddle
                                color: Theme.text
                                font.pixelSize: 13
                                font.weight: Font.DemiBold
                            }

                            CheckBox {
                                text: qsTr("Initialized")
                                checked: submodule.modelData.initialized
                                onToggled: dialog.setSubmoduleInitialized(submodule.index, checked)
                            }

                            PushButton {
                                enabled: submodule.modelData.initialized
                                text: qsTr("Open")
                                onClicked: dialog.openSubmodule(submodule.index)
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 24
                            spacing: 8

                            TextField {
                                Layout.fillWidth: true
                                placeholderText: qsTr("URL")
                                text: submodule.modelData.url
                                onEditingFinished: dialog.setSubmoduleUrl(submodule.index, text)
                            }

                            TextField {
                                Layout.preferredWidth: 140
                                placeholderText: qsTr("Branch")
                                text: submodule.modelData.branch
                                onEditingFinished: dialog.setSubmoduleBranch(submodule.index, text)
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: search

        ColumnLayout {
            spacing: 20

            SettingSection {
                title: qsTr("Index")

                SettingRow {
                    label: qsTr("Indexing")

                    CheckBox {
                        text: qsTr("Index the commits for searching")
                        checked: dialog.indexEnabled
                        onToggled: dialog.indexEnabled = checked
                    }
                }

                SettingRow {
                    label: qsTr("Limit")
                    hint: qsTr("Stop indexing commits after this many terms")

                    SpinBox {
                        implicitWidth: 160
                        enabled: dialog.indexEnabled
                        from: 100000
                        to: 99999999
                        stepSize: 100000
                        value: dialog.termLimit
                        onValueModified: dialog.termLimit = value
                    }
                }

                SettingRow {
                    label: qsTr("Diff context")
                    hint: qsTr("Lines around changes that are indexed")

                    SpinBox {
                        enabled: dialog.indexEnabled
                        from: 0
                        to: 100
                        value: dialog.indexContext
                        onValueModified: dialog.indexContext = value
                    }
                }

                SettingRow {
                    label: qsTr("Remove")

                    PushButton {
                        implicitHeight: 32
                        danger: true
                        enabled: dialog.indexValid
                        text: qsTr("Remove Index")
                        onClicked: dialog.removeIndex()
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
                    hint: qsTr("Enable plugins and set their options for this repository")

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
        id: lfs

        ColumnLayout {
            spacing: 20

            SettingSection {
                visible: !dialog.lfsInitialized
                title: qsTr("Git LFS")

                SettingRow {
                    label: qsTr("Initialize")
                    hint: qsTr("LFS isn't initialized in this repository")

                    PushButton {
                        implicitHeight: 32
                        primary: true
                        text: qsTr("Initialize LFS")
                        onClicked: dialog.initializeLfs()
                    }
                }
            }

            SettingSection {
                visible: dialog.lfsInitialized
                title: qsTr("Tracked patterns")

                SettingRow {
                    label: qsTr("Included")
                    hint: qsTr("Files that match these patterns are stored with LFS")

                    ListFrame {
                        count: dialog.lfsIncluded.length
                        emptyText: qsTr("No patterns")

                        Repeater {
                            model: dialog.lfsIncluded

                            delegate: RowLayout {
                                id: pattern

                                required property string modelData

                                Layout.fillWidth: true
                                Layout.leftMargin: 8

                                Text {
                                    Layout.fillWidth: true
                                    text: pattern.modelData
                                    color: Theme.text
                                    font.family: Theme.monoFont
                                    font.pixelSize: 12
                                }

                                ActionButton {
                                    compact: true
                                    implicitWidth: 26
                                    implicitHeight: 26
                                    icon: "close"
                                    tip: qsTr("Stop tracking")
                                    onClicked: dialog.trackLfs(pattern.modelData, false)
                                }
                            }
                        }
                    }

                    RowLayout {
                        spacing: 8

                        TextField {
                            id: newPattern

                            Layout.fillWidth: true
                            placeholderText: qsTr("*.png, /images/*")
                        }

                        PushButton {
                            implicitHeight: 32
                            enabled: newPattern.text !== ""
                            icon: "plus"
                            text: qsTr("Track")
                            onClicked: {
                                dialog.trackLfs(newPattern.text, true)
                                newPattern.text = ""
                            }
                        }
                    }
                }

                SettingRow {
                    visible: dialog.lfsExcluded.length > 0
                    label: qsTr("Excluded")

                    Text {
                        Layout.fillWidth: true
                        text: dialog.lfsExcluded.join("\n")
                        color: Theme.textMuted
                        font.family: Theme.monoFont
                        font.pixelSize: 12
                    }
                }
            }

            SettingSection {
                visible: dialog.lfsInitialized
                title: qsTr("Transfer")

                SettingRow {
                    label: qsTr("Server URL")

                    TextField {
                        Layout.fillWidth: true
                        text: dialog.lfsSetting("Endpoint")
                        onEditingFinished: dialog.setLfsConfig("lfs.url", text)
                    }
                }

                SettingRow {
                    label: qsTr("Prune offset")
                    hint: qsTr("Days")

                    SpinBox {
                        from: 0
                        to: 3650
                        value: parseInt(dialog.lfsSetting("PruneOffsetDays")) || 0
                        onValueModified: dialog.setLfsConfig("lfs.pruneoffsetdays", value)
                    }
                }

                SettingRow {
                    label: qsTr("Fetch recent")

                    CheckBox {
                        id: fetchRecent

                        text: qsTr("Fetch LFS objects of recent references and commits")
                        checked: dialog.lfsSetting("FetchRecentAlways").indexOf("true") >= 0
                        onToggled: dialog.setLfsConfig("lfs.fetchrecentalways", checked)
                    }

                    RowLayout {
                        spacing: 8

                        SpinBox {
                            enabled: fetchRecent.checked
                            from: 0
                            to: 3650
                            value: parseInt(dialog.lfsSetting("FetchRecentRefsDays")) || 0
                            onValueModified: dialog.setLfsConfig("lfs.fetchrecentrefsdays", value)
                        }

                        Text {
                            text: qsTr("reference days or")
                            color: Theme.textMuted
                            font.pixelSize: 13
                        }

                        SpinBox {
                            enabled: fetchRecent.checked
                            from: 0
                            to: 3650
                            value: parseInt(dialog.lfsSetting("FetchRecentCommitsDays")) || 0
                            onValueModified: dialog.setLfsConfig("lfs.fetchrecentcommitsdays", value)
                        }

                        Text {
                            text: qsTr("commit days")
                            color: Theme.textMuted
                            font.pixelSize: 13
                        }
                    }
                }

                SettingRow {
                    label: qsTr("Advanced")

                    RowLayout {
                        spacing: 8

                        PushButton {
                            implicitHeight: 32
                            text: qsTr("View Environment")
                            onClicked: dialog.showLfsEnvironment()
                        }

                        PushButton {
                            implicitHeight: 32
                            danger: true
                            text: qsTr("Deinitialize LFS")
                            onClicked: dialog.deinitializeLfs()
                        }
                    }
                }
            }
        }
    }
}
