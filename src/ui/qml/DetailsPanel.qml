import QtQuick
import QtQuick.Controls.Basic as Controls
import QtQuick.Layouts
import Gittyup

// The right panel: details of the selected commits, or the staging area and
// commit message for the uncommitted changes. 'detailView' is the C++
// DetailView.
Rectangle {
    id: root

    // Keep in sync with DetailView::Mode and DetailView::List.
    readonly property int noMode: 0
    readonly property int commitMode: 1
    readonly property int rangeMode: 2
    readonly property int wipMode: 3

    readonly property int allFiles: 0
    readonly property int unstagedFiles: 1
    readonly property int stagedFiles: 2

    color: Theme.panel

    Rectangle {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: 1
        color: Theme.border
    }

    Text {
        visible: detailView.mode === root.noMode && !detailView.loading
        anchors.centerIn: parent
        width: parent.width - 40
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        text: qsTr("Select a commit to see its details")
        color: Theme.textMuted
        font.pixelSize: 13
    }

    // Details of a commit or a range of commits.
    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 1
        visible: detailView.mode === root.commitMode || detailView.mode === root.rangeMode
        spacing: 0

        Controls.ScrollView {
            id: infoScroll

            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(info.implicitHeight + 28, root.height * 0.5)
            clip: true
            contentWidth: availableWidth

            ColumnLayout {
                id: info

                x: 14
                y: 14
                width: infoScroll.availableWidth - 28
                spacing: 10

                TextEdit {
                    Layout.fillWidth: true
                    readOnly: true
                    selectByMouse: true
                    wrapMode: TextEdit.Wrap
                    text: detailView.summary
                    color: Theme.text
                    selectionColor: Theme.selected
                    selectedTextColor: Theme.selectedText
                    font.pixelSize: 15
                    font.bold: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    // Avatar or initials.
                    Rectangle {
                        Layout.alignment: Qt.AlignTop
                        implicitWidth: 36
                        implicitHeight: 36
                        radius: 18
                        color: Theme.accent
                        clip: true

                        Text {
                            anchors.centerIn: parent
                            visible: avatar.status !== Image.Ready
                            text: detailView.initials
                            color: Theme.accentText
                            font.pixelSize: 13
                            font.bold: true
                        }

                        Image {
                            id: avatar

                            anchors.fill: parent
                            source: detailView.avatarUrl
                            sourceSize: Qt.size(72, 72)
                            fillMode: Image.PreserveAspectCrop
                            layer.enabled: false
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            Layout.fillWidth: true
                            text: detailView.authorName
                            elide: Text.ElideRight
                            color: Theme.text
                            font.pixelSize: 13
                            font.bold: true
                        }

                        Text {
                            Layout.fillWidth: true
                            visible: detailView.authorEmail !== ""
                            text: detailView.authorEmail
                            elide: Text.ElideRight
                            color: Theme.textMuted
                            font.pixelSize: 11
                        }

                        Text {
                            Layout.fillWidth: true
                            text: detailView.date
                            elide: Text.ElideRight
                            color: Theme.textMuted
                            font.pixelSize: 11
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    visible: !detailView.sameCommitter
                    text: qsTr("Committed by %1 <%2>").arg(detailView.committerName)
                                                      .arg(detailView.committerEmail)
                    elide: Text.ElideRight
                    color: Theme.textMuted
                    font.pixelSize: 11
                }

                // Id and parents.
                Flow {
                    Layout.fillWidth: true
                    spacing: 6

                    Text {
                        height: 22
                        verticalAlignment: Text.AlignVCenter
                        text: detailView.mode === root.rangeMode ? qsTr("range") : qsTr("commit")
                        color: Theme.textMuted
                        font.pixelSize: 11
                    }

                    Rectangle {
                        width: idLabel.implicitWidth + 30
                        height: 22
                        radius: 4
                        color: idMouse.containsMouse ? Theme.hover : Theme.field
                        border.color: Theme.border

                        Row {
                            anchors.centerIn: parent
                            spacing: 5

                            Text {
                                id: idLabel

                                anchors.verticalCenter: parent.verticalCenter
                                text: detailView.shortId
                                color: Theme.text
                                font.pixelSize: 11
                                font.family: Theme.monoFont
                            }

                            Icon {
                                anchors.verticalCenter: parent.verticalCenter
                                name: "log"
                                size: 12
                                color: Theme.textMuted
                            }
                        }

                        MouseArea {
                            id: idMouse

                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: detailView.copyId()
                        }

                        HoverTip {
                            target: parent
                            text: qsTr("Copy the full id")
                            hovered: idMouse.containsMouse
                        }
                    }

                    Text {
                        visible: detailView.mode === root.commitMode
                        height: 22
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: 6
                        text: detailView.parents.length > 1 ? qsTr("parents") : qsTr("parent")
                        color: Theme.textMuted
                        font.pixelSize: 11
                    }

                    Text {
                        visible: detailView.mode === root.commitMode
                                 && detailView.parents.length === 0
                        height: 22
                        verticalAlignment: Text.AlignVCenter
                        text: qsTr("none (initial commit)")
                        color: Theme.textMuted
                        font.pixelSize: 11
                        font.italic: true
                    }

                    Repeater {
                        model: detailView.mode === root.commitMode ? detailView.parents : []

                        Text {
                            required property var modelData

                            height: 22
                            verticalAlignment: Text.AlignVCenter
                            text: modelData.shortId
                            color: parentMouse.containsMouse ? Theme.accent : Theme.text
                            font.pixelSize: 11
                            font.family: Theme.monoFont
                            font.underline: parentMouse.containsMouse

                            MouseArea {
                                id: parentMouse

                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: detailView.selectParent(modelData.id)
                            }
                        }
                    }
                }

                // Signed commits, like GitKraken marks them.
                Rectangle {
                    id: signedChip

                    visible: detailView.mode === root.commitMode && detailView.signature !== ""
                    implicitWidth: signedRow.implicitWidth + 12
                    implicitHeight: 22
                    radius: 4
                    color: Qt.rgba(Theme.added.r, Theme.added.g, Theme.added.b, 0.14)
                    border.color: Qt.rgba(Theme.added.r, Theme.added.g, Theme.added.b, 0.6)

                    Row {
                        id: signedRow

                        anchors.centerIn: parent
                        spacing: 4

                        Icon {
                            anchors.verticalCenter: parent.verticalCenter
                            name: "key"
                            size: 12
                            color: Theme.added
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Signed · %1").arg(detailView.signature)
                            color: Theme.text
                            font.pixelSize: 11
                        }
                    }

                    HoverHandler {
                        id: signedHover
                    }

                    HoverTip {
                        target: signedChip
                        text: qsTr("The commit has a %1 signature. It isn't verified.")
                                  .arg(detailView.signature)
                        hovered: signedHover.hovered
                    }
                }

                Flow {
                    Layout.fillWidth: true
                    visible: detailView.refs.length > 0
                    spacing: 4

                    Repeater {
                        model: detailView.refs

                        RefBadge {
                            required property var modelData

                            ref: modelData
                            maxWidth: info.width
                        }
                    }
                }

                TextEdit {
                    Layout.fillWidth: true
                    visible: text !== ""
                    readOnly: true
                    selectByMouse: true
                    wrapMode: TextEdit.Wrap
                    text: detailView.body
                    color: Theme.text
                    selectionColor: Theme.selected
                    selectedTextColor: Theme.selectedText
                    font.pixelSize: 12
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: Theme.border
        }

        // Changed files, or all files of the commit in tree mode.
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 14
            Layout.rightMargin: 8
            implicitHeight: 36
            spacing: 4

            Text {
                Layout.fillWidth: true
                text: detailView.viewMode === 1
                      ? qsTr("All files")
                      : qsTr("%n changed file(s)", "", detailView.files.fileCount)
                elide: Text.ElideRight
                color: Theme.text
                font.pixelSize: 12
                font.bold: true
            }

            Spinner {
                running: detailView.loading
                size: 14
            }

            FileModeSwitch {}
        }

        FileList {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: detailView.viewMode !== 1
            model: detailView.files
            list: root.allFiles
        }

        FileTree {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: detailView.viewMode === 1
        }
    }

    // Staging area and commit message.
    ColumnLayout {
        id: wip

        anchors.fill: parent
        anchors.leftMargin: 1
        visible: detailView.mode === root.wipMode
        spacing: 0

        property bool syncing: false

        function combined() {
            const description = descriptionField.text.trim()
            return description ? summaryField.text + "\n\n" + description : summaryField.text
        }

        function split(message) {
            syncing = true
            const index = message.indexOf("\n")
            summaryField.text = index < 0 ? message : message.substring(0, index)
            descriptionField.text = index < 0 ? "" : message.substring(index + 1).replace(/^\n+/, "")
            syncing = false
        }

        function edited() {
            if (!syncing)
                detailView.message = combined()
        }

        Component.onCompleted: split(detailView.message)

        // Apply a correction of the spell check.
        Connections {
            target: detailView.spellCheck

            function onReplaceRequested(fieldName, start, length, text) {
                const field = fieldName === "summary" ? summaryField : descriptionField
                field.remove(start, start + length)
                field.insert(start, text)
                wip.edited()
            }
        }

        Connections {
            target: detailView

            function onMessageChanged() {
                if (wip.combined() !== detailView.message)
                    wip.split(detailView.message)
            }

            function onMessagePopulated() {
                summaryField.selectAll()
            }
        }

        // Header.
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 14
            Layout.bottomMargin: 8
            spacing: 3

            Text {
                Layout.fillWidth: true
                text: qsTr("Changes on %1").arg(detailView.branchName)
                elide: Text.ElideRight
                color: Theme.text
                font.pixelSize: 14
                font.bold: true
            }

            Text {
                Layout.fillWidth: true
                text: detailView.statusText
                elide: Text.ElideRight
                color: Theme.textMuted
                font.pixelSize: 11
            }
        }

        // Unstaged files.
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 14
            Layout.rightMargin: 8
            implicitHeight: 34
            spacing: 6

            Text {
                Layout.fillWidth: true
                text: qsTr("Unstaged Files (%1)").arg(detailView.unstagedFiles.fileCount)
                elide: Text.ElideRight
                color: Theme.text
                font.pixelSize: 12
                font.bold: true
            }

            FileModeSwitch {}

            PushButton {
                text: qsTr("Stage All")
                enabled: detailView.canStage
                implicitHeight: 24
                onClicked: detailView.stageFiles(root.unstagedFiles, -1, true)
            }
        }

        FileList {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 60
            model: detailView.unstagedFiles
            list: root.unstagedFiles
            stageAction: 1
            discardable: true
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: Theme.border
        }

        // Staged files.
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 14
            Layout.rightMargin: 8
            implicitHeight: 34
            spacing: 6

            Text {
                Layout.fillWidth: true
                text: qsTr("Staged Files (%1)").arg(detailView.stagedFiles.fileCount)
                elide: Text.ElideRight
                color: Theme.text
                font.pixelSize: 12
                font.bold: true
            }

            PushButton {
                text: qsTr("Unstage All")
                enabled: detailView.canUnstage
                implicitHeight: 24
                onClicked: detailView.stageFiles(root.stagedFiles, -1, false)
            }
        }

        FileList {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 60
            model: detailView.stagedFiles
            list: root.stagedFiles
            stageAction: -1
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: Theme.border
        }

        // Commit message.
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 14
            spacing: 8

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Author: %1").arg(detailView.authorText)
                    elide: Text.ElideRight
                    color: authorMouse.containsMouse ? Theme.accent : Theme.textMuted
                    font.pixelSize: 11

                    MouseArea {
                        id: authorMouse

                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: detailView.changeAuthor()
                    }
                }

                Text {
                    visible: detailView.authorOverridden
                    text: qsTr("reset")
                    color: resetMouse.containsMouse ? Theme.accent : Theme.textMuted
                    font.pixelSize: 11
                    font.underline: true

                    MouseArea {
                        id: resetMouse

                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: detailView.resetAuthor()
                    }
                }

                ActionButton {
                    compact: true
                    icon: "pencil"
                    hasMenu: true
                    menuOnly: true
                    tip: qsTr("Message templates")
                    onMenuRequested: (x, y) => detailView.showTemplateMenu(x, y)
                }
            }

            // Summary.
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 32
                radius: 6
                color: Theme.field
                border.color: summaryField.activeFocus ? Theme.accent : Theme.border

                TextInput {
                    id: summaryField

                    anchors.left: parent.left
                    anchors.right: counter.left
                    anchors.leftMargin: 10
                    anchors.rightMargin: 6
                    anchors.verticalCenter: parent.verticalCenter
                    clip: true
                    selectByMouse: true
                    color: Theme.text
                    selectionColor: Theme.selected
                    selectedTextColor: Theme.selectedText
                    font.pixelSize: 13
                    onTextEdited: wip.edited()
                    Keys.onReturnPressed: descriptionField.forceActiveFocus()

                    SpellUnderlines {
                        field: summaryField
                    }

                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: (eventPoint) => {
                            const p = eventPoint.position
                            detailView.spellCheck.showMenu("summary", summaryField.text,
                                                           summaryField.positionAt(p.x, p.y),
                                                           p.x, p.y)
                        }
                    }

                    Text {
                        anchors.fill: parent
                        verticalAlignment: Text.AlignVCenter
                        visible: !summaryField.text && !summaryField.preeditText
                        text: qsTr("Summary")
                        color: Theme.textMuted
                        font: summaryField.font
                    }
                }

                // Git recommends summaries of at most 72 characters.
                Text {
                    id: counter

                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    text: 72 - summaryField.length
                    color: summaryField.length > 72 ? Theme.deleted : Theme.textMuted
                    font.pixelSize: 11
                }
            }

            // Description.
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 90
                radius: 6
                color: Theme.field
                border.color: descriptionField.activeFocus ? Theme.accent : Theme.border

                Controls.ScrollView {
                    anchors.fill: parent
                    anchors.margins: 1

                    Controls.TextArea {
                        id: descriptionField

                        objectName: "MessageEditor"
                        placeholderText: qsTr("Description")
                        placeholderTextColor: Theme.textMuted
                        wrapMode: TextEdit.Wrap
                        selectByMouse: true
                        color: Theme.text
                        selectionColor: Theme.selected
                        selectedTextColor: Theme.selectedText
                        font.pixelSize: 12
                        leftPadding: 10
                        rightPadding: 10
                        topPadding: 8
                        background: null
                        onTextChanged: wip.edited()

                        SpellUnderlines {
                            field: descriptionField
                        }

                        TapHandler {
                            acceptedButtons: Qt.RightButton
                            onTapped: (eventPoint) => {
                                const p = eventPoint.position
                                detailView.spellCheck.showMenu("description", descriptionField.text,
                                                               descriptionField.positionAt(p.x, p.y),
                                                               p.x, p.y)
                            }
                        }

                        Keys.onPressed: (event) => {
                            if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
                                    && (event.modifiers & Qt.ControlModifier)) {
                                detailView.commitChanges()
                                event.accepted = true
                            }
                        }
                    }
                }
            }

            PushButton {
                Layout.fillWidth: true
                implicitHeight: 34
                primary: true
                text: detailView.commitText
                enabled: detailView.canCommit
                tip: qsTr("Commit the staged changes (Ctrl+Return)")
                onClicked: detailView.commitChanges()
            }

            RowLayout {
                Layout.fillWidth: true
                visible: detailView.rebaseOngoing || detailView.mergeAbortVisible
                spacing: 6

                PushButton {
                    Layout.fillWidth: true
                    visible: detailView.rebaseOngoing
                    text: qsTr("Continue Rebase")
                    onClicked: detailView.continueRebase()
                }

                PushButton {
                    Layout.fillWidth: true
                    visible: detailView.rebaseOngoing
                    danger: true
                    text: qsTr("Abort Rebase")
                    onClicked: detailView.abortRebase()
                }

                PushButton {
                    Layout.fillWidth: true
                    visible: detailView.mergeAbortVisible
                    danger: true
                    text: detailView.mergeAbortText
                    onClicked: detailView.abortMerge()
                }
            }
        }
    }
}
