import QtQuick
import QtQuick.Layouts
import Gittyup

// Find text in the editor or the diff. 'finder' is the C++ FindWidget or
// FindController.
Rectangle {
    id: root

    property QtObject finder: null

    implicitHeight: 44
    color: Theme.panel

    Connections {
        target: root.finder

        function onFocusRequested() {
            field.forceActiveFocus()
            field.selectAll()
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: Theme.border
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 10
        spacing: 8

        Item { Layout.fillWidth: true }

        Text {
            visible: (root.finder?.hitsText ?? "") !== ""
            text: root.finder?.hitsText ?? ""
            color: root.finder?.hasMatches ? Theme.textMuted : Theme.deleted
            font.pixelSize: 12
        }

        ActionButton {
            compact: true
            implicitWidth: 28
            implicitHeight: 28
            enabled: root.finder?.hasMatches ?? false
            icon: "chevron-up"
            tip: qsTr("Previous match")
            onClicked: root.finder.previous()
        }

        ActionButton {
            compact: true
            implicitWidth: 28
            implicitHeight: 28
            enabled: root.finder?.hasMatches ?? false
            icon: "chevron-down"
            tip: qsTr("Next match")
            onClicked: root.finder.next()
        }

        TextField {
            id: field

            Layout.preferredWidth: 260
            implicitHeight: 30
            leftPadding: 30
            placeholderText: qsTr("Find")
            text: root.finder?.searchText ?? ""
            onTextEdited: root.finder.search(text)
            onAccepted: root.finder.next()
            Keys.onEscapePressed: root.finder.hide()

            Icon {
                anchors.left: parent.left
                anchors.leftMargin: 9
                anchors.verticalCenter: parent.verticalCenter
                name: "search"
                size: 14
                color: Theme.textMuted
            }
        }

        PushButton {
            implicitHeight: 30
            text: qsTr("Done")
            onClicked: root.finder.hide()
        }
    }
}
