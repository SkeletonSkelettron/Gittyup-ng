import QtQuick
import QtQuick.Controls.Basic as Controls
import QtQuick.Layouts
import Gittyup

// 'dialog' is the C++ TagDialog.
DialogPage {
    initialFocus: tagNameField
    title: qsTr("Create a tag")
    subtitle: qsTr("The tag points to %1.").arg(dialog.target)
    acceptText: qsTr("Create Tag")
    acceptEnabled: dialog.acceptable

    FormField {
        label: qsTr("Tag name")
        error: dialog.nameError

        TextField {
            id: tagNameField

            Layout.fillWidth: true
            placeholderText: qsTr("v1.0.0")
            text: dialog.name
            error: dialog.nameError !== ""
            onTextEdited: dialog.name = text
        }
    }

    FormField {
        visible: dialog.existingTags.length > 0
        label: dialog.name === "" ? qsTr("Existing tags") : qsTr("Similar tags")

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: Math.min(tagList.contentHeight + 8, 104)
            radius: 6
            color: Theme.base
            border.color: Theme.border

            ListView {
                id: tagList

                anchors.fill: parent
                anchors.margins: 4
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                model: dialog.existingTags

                Controls.ScrollBar.vertical: ThinScrollBar { thickness: 6 }

                delegate: Rectangle {
                    id: tag

                    required property string modelData

                    width: ListView.view.width
                    height: 24
                    radius: 4
                    color: tagMouse.containsMouse ? Theme.hover : "transparent"

                    Row {
                        anchors.left: parent.left
                        anchors.leftMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 8

                        Icon {
                            anchors.verticalCenter: parent.verticalCenter
                            name: "tag"
                            size: 12
                            color: Theme.textMuted
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: tag.modelData
                            color: Theme.text
                            font.pixelSize: 12
                        }
                    }

                    MouseArea {
                        id: tagMouse

                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        // Start from an existing tag.
                        onClicked: dialog.name = tag.modelData
                    }
                }
            }
        }
    }

    CheckBox {
        text: qsTr("Force (replace an existing tag)")
        checked: dialog.force
        onToggled: dialog.force = checked
    }

    CheckBox {
        visible: dialog.pushText !== ""
        text: dialog.pushText
        checked: dialog.push
        onToggled: dialog.push = checked
    }

    CheckBox {
        text: qsTr("Annotated tag with a message")
        checked: dialog.annotated
        onToggled: dialog.annotated = checked
    }

    TextArea {
        Layout.fillWidth: true
        visible: dialog.annotated
        placeholderText: qsTr("Message")
        text: dialog.message
        onTextEdited: dialog.message = text
    }
}
