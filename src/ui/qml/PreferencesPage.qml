import QtQuick
import QtQuick.Controls.Basic as Controls
import QtQuick.Layouts
import Gittyup

// Settings in sections, with a sidebar to switch between them. Each section
// has a 'title', an 'icon' and a 'description', and 'pages' holds the
// component of each section.
Rectangle {
    id: root

    property string title
    property var sections: []
    property var pages: []
    property int current: 0
    // Sections that are left out of the sidebar.
    property var hiddenSections: []
    property string sideButtonText
    property string sideButtonIcon
    property string sideButtonTip

    // Called by QmlDialog once the dialog is shown. Nothing is edited yet.
    function focusInitialItem() {
        root.forceActiveFocus()
    }

    signal sectionSelected(int index)
    signal sideButtonClicked()
    signal closeRequested()

    implicitWidth: 880
    implicitHeight: 620
    color: Theme.panel
    focus: true

    Keys.onEscapePressed: root.closeRequested()

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Navigation.
        Rectangle {
            Layout.preferredWidth: 210
            Layout.fillHeight: true
            color: Theme.sidebar

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 2

                Text {
                    Layout.fillWidth: true
                    Layout.leftMargin: 8
                    Layout.topMargin: 6
                    Layout.bottomMargin: 12
                    text: root.title
                    elide: Text.ElideRight
                    color: Theme.text
                    font.pixelSize: 18
                    font.weight: Font.Bold
                }

                Repeater {
                    model: root.sections

                    delegate: Rectangle {
                        id: navItem

                        required property int index
                        required property var modelData

                        readonly property bool currentItem: root.current === index

                        Layout.fillWidth: true
                        visible: root.hiddenSections.indexOf(index) < 0
                        implicitHeight: 34
                        radius: 6
                        color: currentItem ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.16)
                                           : navMouse.containsMouse ? Theme.hover : "transparent"

                        Rectangle {
                            visible: navItem.currentItem
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            width: 3
                            height: 16
                            radius: 1.5
                            color: Theme.accent
                        }

                        Row {
                            anchors.left: parent.left
                            anchors.leftMargin: 12
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 10

                            Icon {
                                anchors.verticalCenter: parent.verticalCenter
                                name: navItem.modelData.icon
                                size: 16
                                color: navItem.currentItem ? Theme.accent : Theme.textMuted
                            }

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: navItem.modelData.title
                                color: navItem.currentItem ? Theme.text : Theme.textMuted
                                font.pixelSize: 13
                                font.weight: navItem.currentItem ? Font.DemiBold : Font.Normal
                            }
                        }

                        MouseArea {
                            id: navMouse

                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.sectionSelected(navItem.index)
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                PushButton {
                    Layout.fillWidth: true
                    visible: root.sideButtonText !== ""
                    icon: root.sideButtonIcon
                    text: root.sideButtonText
                    tip: root.sideButtonTip
                    onClicked: root.sideButtonClicked()
                }
            }
        }

        Rectangle {
            Layout.fillHeight: true
            width: 1
            color: Theme.border
        }

        // Content.
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 28
                Layout.rightMargin: 28
                Layout.topMargin: 24
                Layout.bottomMargin: 12
                spacing: 4

                Text {
                    text: root.sections.length ? root.sections[root.current].title : ""
                    color: Theme.text
                    font.pixelSize: 20
                    font.weight: Font.Bold
                }

                Text {
                    Layout.fillWidth: true
                    text: root.sections.length ? root.sections[root.current].description : ""
                    wrapMode: Text.Wrap
                    color: Theme.textMuted
                    font.pixelSize: 12
                }
            }

            Flickable {
                id: flickable

                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentHeight: loader.height + 32
                boundsBehavior: Flickable.StopAtBounds

                Controls.ScrollBar.vertical: ThinScrollBar {}

                Loader {
                    id: loader

                    x: 28
                    y: 8
                    width: flickable.width - 56
                    sourceComponent: root.pages[root.current]
                    onLoaded: flickable.contentY = 0
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.border
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 14
                Layout.leftMargin: 28
                Layout.rightMargin: 20

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Changes are saved right away.")
                    color: Theme.textMuted
                    font.pixelSize: 12
                }

                PushButton {
                    implicitHeight: 32
                    minimumWidth: 88
                    primary: true
                    text: qsTr("Done")
                    onClicked: root.closeRequested()
                }
            }
        }
    }
}
