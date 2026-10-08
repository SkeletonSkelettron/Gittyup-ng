import QtQuick
import QtQuick.Controls.Basic as Controls
import QtQuick.Layouts
import Gittyup

// 'dialog' is the C++ ExternalToolsDialog.
DialogPage {
    title: dialog.type === "merge" ? qsTr("External merge tools") : qsTr("External diff tools")
    subtitle: qsTr("Tools that %1 found and tools that you added to the global git configuration.").arg(Qt.application.name)
    contentWidth: 680
    acceptText: qsTr("Done")
    rejectVisible: false

    component SectionTitle: Text {
        color: Theme.textMuted
        font.pixelSize: 11
        font.weight: Font.DemiBold
        font.letterSpacing: 0.8
        font.capitalization: Font.AllUppercase
    }

    SectionTitle { text: qsTr("Detected tools") }

    Rectangle {
        Layout.fillWidth: true
        implicitHeight: Math.min(detectedList.contentHeight + 8, 200)
        radius: 8
        color: Theme.base
        border.color: Theme.border

        ListView {
            id: detectedList

            anchors.fill: parent
            anchors.margins: 4
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: dialog.detected

            Controls.ScrollBar.vertical: ThinScrollBar { thickness: 6 }

            delegate: RowLayout {
                id: detectedTool

                required property var modelData

                width: ListView.view.width
                height: 30
                spacing: 10
                opacity: modelData.found ? 1 : 0.45

                Icon {
                    Layout.leftMargin: 8
                    name: detectedTool.modelData.found ? "check" : "minus"
                    size: 14
                    color: detectedTool.modelData.found ? Theme.accent : Theme.textMuted
                }

                Text {
                    Layout.preferredWidth: 120
                    text: detectedTool.modelData.name
                    elide: Text.ElideRight
                    color: Theme.text
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }

                Text {
                    Layout.fillWidth: true
                    text: detectedTool.modelData.command + " " + detectedTool.modelData.arguments
                    elide: Text.ElideMiddle
                    color: Theme.textMuted
                    font.family: Theme.monoFont
                    font.pixelSize: 12
                }
            }
        }
    }

    SectionTitle { text: qsTr("User defined tools") }

    Rectangle {
        Layout.fillWidth: true
        implicitHeight: dialog.userDefined.length ? userColumn.implicitHeight + 8 : 56
        radius: 8
        color: Theme.base
        border.color: Theme.border

        Text {
            visible: dialog.userDefined.length === 0
            anchors.centerIn: parent
            text: qsTr("Add a tool to use it for diffs or merges.")
            color: Theme.textMuted
            font.pixelSize: 12
        }

        ColumnLayout {
            id: userColumn

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 4
            spacing: 2

            Repeater {
                model: dialog.userDefined

                delegate: RowLayout {
                    id: userTool

                    required property int index
                    required property var modelData

                    Layout.fillWidth: true
                    Layout.margins: 4
                    spacing: 8

                    TextField {
                        Layout.preferredWidth: 120
                        placeholderText: qsTr("Name")
                        text: userTool.modelData.name
                        onEditingFinished: dialog.setToolValue(userTool.index, 0, text)
                    }

                    TextField {
                        Layout.fillWidth: true
                        placeholderText: qsTr("Command")
                        text: userTool.modelData.command
                        font.family: Theme.monoFont
                        onEditingFinished: dialog.setToolValue(userTool.index, 1, text)
                    }

                    TextField {
                        Layout.preferredWidth: 180
                        placeholderText: qsTr("Arguments")
                        text: userTool.modelData.arguments
                        font.family: Theme.monoFont
                        onEditingFinished: dialog.setToolValue(userTool.index, 2, text)
                    }

                    ActionButton {
                        compact: true
                        implicitWidth: 30
                        implicitHeight: 30
                        icon: "close"
                        foregroundOverride: Theme.deleted
                        tip: qsTr("Remove the tool")
                        onClicked: dialog.removeTool(userTool.index)
                    }
                }
            }
        }
    }

    PushButton {
        implicitHeight: 32
        icon: "plus"
        text: qsTr("Add Tool...")
        onClicked: dialog.addTool()
    }
}
