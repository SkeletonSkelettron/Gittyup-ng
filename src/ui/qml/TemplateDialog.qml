import QtQuick
import QtQuick.Controls.Basic as Controls
import QtQuick.Layouts
import Gittyup

// 'dialog' is the C++ TemplateDialog.
DialogPage {
    title: qsTr("Commit message templates")
    subtitle: qsTr("The first template fills in the message automatically.")
    acceptText: qsTr("Save")
    contentWidth: 640
    customAccept: true
    initialFocus: nameField
    onAcceptRequested: dialog.applyTemplates()

    extraButtons: [
        PushButton {
            implicitHeight: 32
            text: qsTr("Import")
            onClicked: dialog.importTemplates()
        },
        PushButton {
            implicitHeight: 32
            text: qsTr("Export")
            onClicked: dialog.exportTemplates()
        }
    ]

    RowLayout {
        Layout.fillWidth: true
        spacing: 16

        // Templates.
        ColumnLayout {
            Layout.preferredWidth: 220
            Layout.fillHeight: true
            spacing: 8

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 220
                radius: 6
                color: Theme.base
                border.color: Theme.border

                Text {
                    visible: listView.count === 0
                    anchors.centerIn: parent
                    text: qsTr("No templates yet")
                    color: Theme.textMuted
                    font.pixelSize: 12
                }

                ListView {
                    id: listView

                    anchors.fill: parent
                    anchors.margins: 4
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    model: dialog.names

                    Controls.ScrollBar.vertical: ThinScrollBar { thickness: 6 }

                    delegate: Rectangle {
                        id: delegateItem

                        required property int index
                        required property string modelData

                        width: ListView.view.width
                        height: 28
                        radius: 4
                        color: index === dialog.current ? Theme.selected
                               : itemMouse.containsMouse ? Theme.hover : "transparent"

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 10
                            anchors.right: parent.right
                            anchors.rightMargin: 10
                            anchors.verticalCenter: parent.verticalCenter
                            text: delegateItem.modelData
                            elide: Text.ElideRight
                            color: delegateItem.index === dialog.current ? Theme.selectedText : Theme.text
                            font.pixelSize: 12
                            font.weight: delegateItem.index === 0 ? Font.DemiBold : Font.Normal
                        }

                        MouseArea {
                            id: itemMouse

                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: dialog.current = delegateItem.index
                        }
                    }
                }
            }

            RowLayout {
                spacing: 4

                ActionButton {
                    compact: true
                    implicitWidth: 28
                    implicitHeight: 28
                    icon: "chevron-up"
                    tip: qsTr("Move up")
                    enabled: dialog.current > 0
                    onClicked: dialog.moveTemplateUp()
                }

                ActionButton {
                    compact: true
                    implicitWidth: 28
                    implicitHeight: 28
                    icon: "chevron-down"
                    tip: qsTr("Move down")
                    enabled: dialog.current >= 0 && dialog.current < dialog.names.length - 1
                    onClicked: dialog.moveTemplateDown()
                }

                Item { Layout.fillWidth: true }

                PushButton {
                    danger: true
                    text: qsTr("Remove")
                    enabled: dialog.current >= 0
                    onClicked: dialog.removeTemplate()
                }
            }
        }

        // Editor.
        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            spacing: 12

            FormField {
                label: qsTr("Name")

                TextField {
                    id: nameField

                    Layout.fillWidth: true
                    text: dialog.name
                    onTextEdited: dialog.name = text
                }
            }

            FormField {
                label: qsTr("Content")
                hint: dialog.cursorHint

                TextArea {
                    Layout.fillWidth: true
                    implicitHeight: 150
                    text: dialog.templateText
                    font.family: Theme.monoFont
                    onTextEdited: dialog.templateText = text
                }
            }

            PushButton {
                Layout.alignment: Qt.AlignRight
                enabled: dialog.name !== ""
                icon: dialog.nameExists ? "check" : "plus"
                text: dialog.nameExists ? qsTr("Update Template") : qsTr("Add Template")
                onClicked: dialog.addTemplate()
            }
        }
    }
}
