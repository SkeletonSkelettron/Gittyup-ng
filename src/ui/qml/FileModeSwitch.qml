import QtQuick
import Gittyup

// Switch between a flat list and a tree of the changed files.
Row {
    spacing: 0

    ActionButton {
        compact: true
        icon: "log"
        checked: detailView.listMode
        tip: qsTr("Show files as a list")
        implicitWidth: 26
        implicitHeight: 24
        onClicked: detailView.setListMode(true)
    }

    ActionButton {
        compact: true
        icon: "view-tree"
        checked: !detailView.listMode
        tip: qsTr("Show files as a tree")
        implicitWidth: 26
        implicitHeight: 24
        onClicked: detailView.setListMode(false)
    }

    ActionButton {
        compact: true
        icon: "more"
        hasMenu: true
        menuOnly: true
        tip: qsTr("Options")
        implicitWidth: 26
        implicitHeight: 24
        onMenuRequested: (x, y) => detailView.showOptionsMenu(x, y)
    }
}
