import QtQuick
import QtQuick.Layouts
import Gittyup

// Main window toolbar. 'toolbar' is the C++ ToolBar that owns this view.
Rectangle {
    id: root

    // Hide button labels when there isn't enough room for them.
    readonly property bool narrow: width < 1408
    // Hide buttons that duplicate other controls when space is tight.
    readonly property bool tight: width < 1228
    // Also hide what the View menu covers when space is really tight.
    readonly property bool cramped: width < 1018
    // Make the search field narrower.
    readonly property bool narrowSearch: width < 1100

    color: Theme.toolbar

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 6
        anchors.rightMargin: 4
        spacing: 2

        ActionButton {
            compact: true
            icon: "sidebar"
            checked: toolbar.sidebarVisible
            tip: toolbar.sidebarVisible ? qsTr("Hide repository sidebar")
                                        : qsTr("Show repository sidebar")
            onClicked: toolbar.toggleSideBar()
        }

        Separator {}

        HeadChip {
            visible: !root.cramped
            maxTextWidth: 160
            caption: qsTr("Repository")
            value: toolbar.repoName !== "" ? toolbar.repoName : qsTr("None")
            icon: "repo"
            showChevron: false
            tip: toolbar.repoPath
            onClicked: toolbar.toggleSideBar()
        }

        Icon {
            visible: toolbar.hasView && !root.cramped
            name: "chevron-right"
            size: 12
            color: Theme.textMuted
        }

        HeadChip {
            visible: toolbar.hasView
            maxTextWidth: 200
            enabled: toolbar.canCheckout
            caption: qsTr("Branch")
            value: toolbar.branchName
            icon: "branch"
            tip: qsTr("Checkout")
            onClicked: toolbar.checkout()
        }

        Item { Layout.fillWidth: true }

        ActionButton {
            visible: !root.tight
            icon: "arrow-left"
            text: qsTr("Back")
            compact: root.narrow
            hasMenu: true
            showChevron: false
            enabled: toolbar.canPrev
            tip: qsTr("Previous")
            onClicked: toolbar.prev()
            onMenuRequested: (x, y) => toolbar.showHistoryMenu(false, x, y)
        }

        ActionButton {
            visible: !root.tight
            icon: "arrow-right"
            text: qsTr("Forward")
            compact: root.narrow
            hasMenu: true
            showChevron: false
            enabled: toolbar.canNext
            tip: qsTr("Next")
            onClicked: toolbar.next()
            onMenuRequested: (x, y) => toolbar.showHistoryMenu(true, x, y)
        }

        Separator { visible: !root.tight }

        // Undo and redo the last actions in the repository, like GitKraken.
        ActionButton {
            icon: "undo"
            text: qsTr("Undo")
            compact: root.narrow
            enabled: toolbar.canUndo
            tip: toolbar.canUndo ? qsTr("Undo %1").arg(toolbar.undoText)
                                 : qsTr("Undo")
            onClicked: toolbar.undo()
        }

        ActionButton {
            icon: "redo"
            text: qsTr("Redo")
            compact: root.narrow
            enabled: toolbar.canRedo
            tip: toolbar.canRedo ? qsTr("Redo %1").arg(toolbar.redoText)
                                 : qsTr("Redo")
            onClicked: toolbar.redo()
        }

        Separator {}

        ActionButton {
            icon: "fetch"
            text: qsTr("Fetch")
            compact: root.narrow
            enabled: toolbar.hasView
            tip: qsTr("Fetch")
            onClicked: toolbar.fetch()
        }

        ActionButton {
            icon: "pull"
            text: qsTr("Pull")
            compact: root.narrow
            hasMenu: true
            badge: toolbar.behind
            badgeColor: Theme.behind
            enabled: toolbar.canPull
            tip: qsTr("Pull")
            onClicked: toolbar.pull()
            onMenuRequested: (x, y) => toolbar.showPullMenu(x, y)
        }

        ActionButton {
            icon: "push"
            text: qsTr("Push")
            compact: root.narrow
            badge: toolbar.ahead
            badgeColor: Theme.ahead
            enabled: toolbar.hasView
            tip: qsTr("Push")
            onClicked: toolbar.push()
        }

        Separator {}

        ActionButton {
            visible: !root.tight
            icon: "branch"
            text: qsTr("Checkout")
            compact: root.narrow
            enabled: toolbar.canCheckout
            tip: qsTr("Checkout")
            onClicked: toolbar.checkout()
        }

        ActionButton {
            icon: "stash"
            text: qsTr("Stash")
            compact: root.narrow
            enabled: toolbar.canStash
            tip: qsTr("Stash")
            onClicked: toolbar.stash()
        }

        ActionButton {
            icon: "pop"
            text: qsTr("Pop")
            compact: root.narrow
            enabled: toolbar.canPop
            tip: qsTr("Pop Stash")
            onClicked: toolbar.popStash()
        }

        Separator {}

        ActionButton {
            icon: "refresh"
            text: qsTr("Refresh")
            compact: root.narrow
            enabled: toolbar.hasView
            tip: qsTr("Refresh")
            onClicked: toolbar.refresh()
        }

        ActionButton {
            visible: toolbar.pullRequestAvailable
            icon: "pull-request"
            text: qsTr("PR")
            compact: root.narrow
            enabled: toolbar.hasView
            tip: qsTr("Create Pull Request")
            onClicked: toolbar.createPullRequest()
        }

        Item { Layout.fillWidth: true }

        ActionButton {
            compact: true
            icon: "terminal"
            enabled: toolbar.hasView
            tip: qsTr("Open Terminal")
            onClicked: toolbar.openTerminal()
        }

        ActionButton {
            visible: !root.tight
            compact: true
            icon: "folder"
            enabled: toolbar.hasView
            tip: qsTr("Open file manager")
            onClicked: toolbar.openFileManager()
        }

        ActionButton {
            visible: !root.cramped
            compact: true
            icon: "log"
            enabled: toolbar.hasView
            checked: toolbar.logVisible
            tip: toolbar.logVisible ? qsTr("Hide Log") : qsTr("Show Log")
            onClicked: toolbar.toggleLog()
        }

        // View mode. The order matches RepoView::ViewMode.
        Rectangle {
            visible: !root.cramped
            Layout.leftMargin: 4
            Layout.rightMargin: 4
            Layout.minimumWidth: implicitWidth
            implicitWidth: modes.implicitWidth + 4
            implicitHeight: modes.implicitHeight + 4
            radius: 8
            color: "transparent"
            border.color: Theme.border

            Row {
                id: modes

                anchors.centerIn: parent

                ActionButton {
                    compact: true
                    icon: "view-double"
                    enabled: toolbar.hasView
                    checked: toolbar.viewMode === 0
                    tip: qsTr("Double Tree View")
                    onClicked: toolbar.setViewMode(0)
                }

                ActionButton {
                    compact: true
                    icon: "view-tree"
                    enabled: toolbar.hasView
                    checked: toolbar.viewMode === 1
                    tip: qsTr("Tree View")
                    onClicked: toolbar.setViewMode(1)
                }
            }
        }

        ActionButton {
            visible: !root.cramped || toolbar.starred
            compact: true
            icon: toolbar.starred ? "star-filled" : "star"
            enabled: toolbar.hasView
            foregroundOverride: toolbar.starred ? Theme.star : "transparent"
            tip: qsTr("Show Starred Commits")
            onClicked: toolbar.setStarred(!toolbar.starred)
        }

        ActionButton {
            compact: true
            icon: "settings"
            hasMenu: true
            menuOnly: true
            tip: qsTr("Configure Settings")
            onMenuRequested: (x, y) => toolbar.showSettingsMenu(x, y)
        }

        SearchField {
            Layout.leftMargin: 6
            Layout.rightMargin: 4
            Layout.preferredWidth: root.narrowSearch ? 150 : 220
        }
    }
}
