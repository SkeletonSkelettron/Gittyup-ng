import QtQuick
import QtQuick.Controls.Basic as Controls
import QtQuick.Layouts
import Gittyup

// 'dialog' is the C++ UpdateSubmodulesDialog.
DialogPage {
    title: qsTr("Update submodules")
    subtitle: qsTr("Check out the commits that the repository records for its submodules.")
    acceptText: qsTr("Update")
    acceptEnabled: dialog.acceptable

    RowLayout {
        Layout.fillWidth: true

        Text {
            Layout.fillWidth: true
            text: qsTr("%1 submodules").arg(dialog.submodules.length)
            color: Theme.textMuted
            font.pixelSize: 12
            font.weight: Font.DemiBold
        }

        PushButton {
            text: qsTr("Select All")
            onClicked: dialog.setAllEnabled(true)
        }

        PushButton {
            text: qsTr("Select None")
            onClicked: dialog.setAllEnabled(false)
        }
    }

    Rectangle {
        Layout.fillWidth: true
        implicitHeight: Math.min(listView.contentHeight + 8, 260)
        radius: 6
        color: Theme.base
        border.color: Theme.border

        ListView {
            id: listView

            anchors.fill: parent
            anchors.margins: 4
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: dialog.submodules

            Controls.ScrollBar.vertical: ThinScrollBar { thickness: 6 }

            delegate: Rectangle {
                id: delegateItem

                required property int index
                required property var modelData

                width: ListView.view.width
                height: 30
                radius: 4
                color: itemMouse.containsMouse ? Theme.hover : "transparent"

                CheckBox {
                    anchors.left: parent.left
                    anchors.leftMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    text: delegateItem.modelData.name
                    checked: delegateItem.modelData.enabled
                    onToggled: dialog.setEnabled(delegateItem.index, checked)
                }

                Text {
                    visible: delegateItem.modelData.path !== delegateItem.modelData.name
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    text: delegateItem.modelData.path
                    color: Theme.textMuted
                    font.pixelSize: 12
                }

                MouseArea {
                    id: itemMouse

                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.NoButton
                }
            }
        }
    }

    CheckBox {
        text: qsTr("Update nested submodules recursively")
        checked: dialog.recursive
        onToggled: dialog.recursive = checked
    }

    CheckBox {
        text: qsTr("Initialize submodules that aren't initialized yet")
        checked: dialog.init
        onToggled: dialog.init = checked
    }
}
