import QtQuick
import QtQuick.Layouts
import Gittyup

// The root item of QML dialogs, with a title, the content and a row of
// buttons. 'dialog' is the C++ QmlDialog.
Rectangle {
    id: root

    property string title
    property string subtitle
    property string acceptText: qsTr("OK")
    property string rejectText: qsTr("Cancel")
    property bool acceptEnabled: true
    property bool acceptVisible: true
    property bool rejectVisible: true
    // The accept button destroys or discards something.
    property bool danger: false
    // The width of the content. Text wraps within it.
    property int contentWidth: 420

    default property alias content: body.data
    // Buttons on the left of the footer.
    property alias extraButtons: extra.data

    // The item that has the focus when the dialog opens.
    property Item initialFocus: null

    // Called by QmlDialog once the dialog is shown.
    function focusInitialItem() {
        (root.initialFocus || root).forceActiveFocus()
    }

    // Handle the accept button instead of accepting the dialog.
    property bool customAccept: false
    signal acceptRequested()

    function accept() {
        if (!root.acceptVisible || !root.acceptEnabled)
            return
        if (root.customAccept)
            root.acceptRequested()
        else
            dialog.accept()
    }

    implicitWidth: root.contentWidth + 48
    implicitHeight: column.implicitHeight + 44
    color: Theme.panel
    focus: true

    Keys.onEscapePressed: dialog.reject()
    Keys.onReturnPressed: root.accept()
    Keys.onEnterPressed: root.accept()

    ColumnLayout {
        id: column

        anchors.fill: parent
        anchors.leftMargin: 24
        anchors.rightMargin: 24
        anchors.topMargin: 20
        anchors.bottomMargin: 24
        spacing: 18

        ColumnLayout {
            Layout.fillWidth: true
            visible: root.title !== ""
            spacing: 4

            Text {
                Layout.fillWidth: true
                text: root.title
                elide: Text.ElideRight
                color: Theme.text
                font.pixelSize: 16
                font.weight: Font.DemiBold
            }

            Text {
                Layout.fillWidth: true
                visible: root.subtitle !== ""
                text: root.subtitle
                wrapMode: Text.Wrap
                color: Theme.textMuted
                font.pixelSize: 12
            }
        }

        ColumnLayout {
            id: body

            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 14
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            RowLayout {
                id: extra

                spacing: 8
            }

            Item { Layout.fillWidth: true }

            PushButton {
                visible: root.rejectVisible
                implicitHeight: 32
                minimumWidth: 88
                text: root.rejectText
                onClicked: dialog.reject()
            }

            PushButton {
                visible: root.acceptVisible
                implicitHeight: 32
                minimumWidth: 88
                primary: true
                danger: root.danger
                enabled: root.acceptEnabled
                text: root.acceptText
                onClicked: root.accept()
            }
        }
    }
}
