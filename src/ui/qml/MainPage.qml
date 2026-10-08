import QtQuick
import QtQuick.Layouts
import Gittyup

// The main window: the repository tabs, the tool bar, the repository
// sidebar and the page of the current repository, or the welcome page.
// 'mainWindow' is the C++ MainWindow. It adds the pages of the repositories
// to 'pages', each with its own context.
Rectangle {
    id: root

    readonly property int sideBarWidth: 240

    color: Theme.base

    ToolTipPopup {}

    // Ctrl+P finds commands, branches, files and repositories.
    CommandPalette {
        anchors.fill: parent
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        MenuBarRow {
            Layout.fillWidth: true
            target: mainWindow
        }

        TabStrip {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
        }

        ToolBar {
            Layout.fillWidth: true
            Layout.preferredHeight: 51
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.border
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // The sidebar slides in and out from the left.
            Item {
                id: sideBarArea

                Layout.fillHeight: true
                Layout.preferredWidth: mainWindow.sideBarVisible ? root.sideBarWidth : 0
                visible: Layout.preferredWidth > 0
                clip: true

                Behavior on Layout.preferredWidth {
                    NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
                }

                FocusScope {
                    anchors.right: parent.right
                    width: root.sideBarWidth
                    height: parent.height

                    SideBar {
                        anchors.fill: parent
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                // The pages of the repositories.
                Item {
                    id: pages

                    objectName: "pages"
                    anchors.fill: parent
                }

                FocusScope {
                    id: welcomeScope

                    anchors.fill: parent
                    visible: mainWindow.welcomeVisible
                    onVisibleChanged: {
                        if (visible)
                            forceActiveFocus()
                    }
                    Component.onCompleted: {
                        if (visible)
                            forceActiveFocus()
                    }

                    WelcomePage {
                        anchors.fill: parent
                    }
                }
            }
        }
    }
}
