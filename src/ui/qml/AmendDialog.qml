import QtQuick
import QtQuick.Layouts
import Gittyup

// 'dialog' is the C++ AmendDialog.
DialogPage {
    title: qsTr("Amend commit")
    subtitle: qsTr("Change the author, the committer or the message of the commit.")
    acceptText: qsTr("Amend")
    acceptEnabled: dialog.acceptable
    contentWidth: 620

    // Keep in sync with ContributorInfo::SelectedDateTimeType.
    readonly property var dateTypes: [qsTr("Current"), qsTr("Manual"), qsTr("Original")]

    component Contributor: Rectangle {
        id: box

        property var contributor

        Layout.fillWidth: true
        Layout.preferredWidth: 1
        Layout.alignment: Qt.AlignTop
        implicitHeight: fields.implicitHeight + 28
        radius: 8
        color: Theme.base
        border.color: Theme.border

        ColumnLayout {
            id: fields

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 14
            spacing: 12

            Text {
                text: box.contributor.title
                color: Theme.text
                font.pixelSize: 13
                font.weight: Font.DemiBold
            }

            FormField {
                label: qsTr("Name")

                TextField {
                    Layout.fillWidth: true
                    text: box.contributor.name
                    error: box.contributor.name === ""
                    onTextEdited: box.contributor.name = text
                }
            }

            FormField {
                label: qsTr("Email")

                TextField {
                    Layout.fillWidth: true
                    text: box.contributor.email
                    onTextEdited: box.contributor.email = text
                }
            }

            FormField {
                label: qsTr("Date")
                hint: box.contributor.dateType === 2 ? box.contributor.originalDate : ""

                SegmentedControl {
                    model: dateTypes
                    currentIndex: box.contributor.dateType
                    onActivated: (index) => box.contributor.dateType = index
                }

                TextField {
                    Layout.fillWidth: true
                    visible: box.contributor.dateType === 1
                    placeholderText: "yyyy-MM-dd HH:mm:ss"
                    text: box.contributor.dateText
                    error: !box.contributor.dateValid
                    onTextEdited: box.contributor.dateText = text
                }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 14

        Contributor { contributor: dialog.author }
        Contributor { contributor: dialog.committer }
    }

    FormField {
        label: qsTr("Message")

        TextArea {
            Layout.fillWidth: true
            implicitHeight: 120
            text: dialog.message
            font.family: Theme.monoFont
            onTextEdited: dialog.message = text
        }
    }
}
