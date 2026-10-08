import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as Controls
import Gittyup

// The panel below the search field that builds a query from fields.
// 'search' is the C++ SearchField.
Rectangle {
    id: root

    implicitHeight: column.implicitHeight + 32
    color: Theme.panel
    border.color: Theme.border
    focus: true

    Keys.onEscapePressed: search.hideAdvanced()
    Keys.onReturnPressed: search.acceptAdvanced()
    Keys.onEnterPressed: search.acceptAdvanced()

    ColumnLayout {
        id: column

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 16
        spacing: 8

        Text {
            Layout.bottomMargin: 4
            text: qsTr("Advanced Search")
            color: Theme.text
            font.pixelSize: 14
            font.weight: Font.DemiBold
        }

        Repeater {
            id: fields

            model: search.advancedFields

            delegate: ColumnLayout {
                id: row

                required property int index
                required property var modelData
                readonly property alias field: field

                Layout.fillWidth: true
                spacing: 8

                Rectangle {
                    visible: row.modelData.group
                    Layout.fillWidth: true
                    Layout.topMargin: 2
                    Layout.bottomMargin: 2
                    implicitHeight: 1
                    color: Theme.border
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        id: label

                        Layout.preferredWidth: 70
                        text: row.modelData.label
                        elide: Text.ElideRight
                        color: Theme.textMuted
                        font.pixelSize: 12

                        HoverHandler {
                            id: labelHover
                        }

                        HoverTip {
                            target: label
                            text: row.modelData.tip
                            hovered: labelHover.hovered
                        }
                    }

                    TextField {
                        id: field

                        Layout.fillWidth: true
                        implicitHeight: 28
                        font.pixelSize: 12
                        text: row.modelData.value
                        placeholderText: row.modelData.tip
                        onTextEdited: {
                            search.setAdvancedValue(row.index, text)
                            completions.update(field, row.index)
                        }
                        onActiveFocusChanged: {
                            if (!activeFocus && completions.field === field)
                                completions.close()
                        }

                        Keys.onDownPressed: (event) => event.accepted = completions.move(field, 1)
                        Keys.onUpPressed: (event) => event.accepted = completions.move(field, -1)
                        Keys.onReturnPressed: (event) => event.accepted = completions.accept(field)
                        Keys.onEnterPressed: (event) => event.accepted = completions.accept(field)
                        Keys.onEscapePressed: (event) => {
                            event.accepted = completions.opened
                            completions.close()
                        }
                    }

                    ActionButton {
                        visible: row.modelData.date
                        compact: true
                        implicitWidth: 28
                        implicitHeight: 28
                        icon: "calendar"
                        tip: qsTr("Pick a date")
                        onClicked: calendar.show(field, row.index)
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 8
            spacing: 8

            Item {
                Layout.fillWidth: true
            }

            PushButton {
                text: qsTr("Cancel")
                minimumWidth: 80
                onClicked: search.hideAdvanced()
            }

            PushButton {
                text: qsTr("Search")
                primary: true
                minimumWidth: 80
                onClicked: search.acceptAdvanced()
            }
        }
    }

    // Drop down below a field, or above it when there's no room below.
    function place(popupItem, field) {
        const p = field.mapToItem(root, 0, 0)
        const below = p.y + field.height + 2
        popupItem.x = p.x
        popupItem.y = below + popupItem.height <= root.height ? below
                                                              : p.y - popupItem.height - 2
    }

    // The values of the index that start with the text of a field.
    Controls.Popup {
        id: completions

        property Item field: null
        property int fieldIndex: -1
        property var items: []
        property int current: -1

        function update(field, index) {
            completions.field = field
            completions.fieldIndex = index
            completions.items = search.advancedCompletions(index, field.text)
            completions.current = -1
            if (completions.items.length === 0) {
                completions.close()
                return
            }

            completions.width = field.width
            root.place(completions, field)
            completions.open()
        }

        function move(field, delta) {
            if (!completions.opened || completions.field !== field)
                return false
            completions.current = Math.max(-1, Math.min(completions.items.length - 1,
                                                         completions.current + delta))
            if (completions.current >= 0)
                listView.positionViewAtIndex(completions.current, ListView.Contain)
            return true
        }

        function accept(field) {
            if (!completions.opened || completions.field !== field || completions.current < 0)
                return false
            completions.apply(completions.current)
            return true
        }

        function apply(index) {
            const text = completions.items[index]
            completions.field.text = text
            search.setAdvancedValue(completions.fieldIndex, text)
            completions.close()
        }

        height: Math.min(completions.items.length, 6) * 26 + 8
        padding: 4
        margins: 0
        focus: false
        closePolicy: Controls.Popup.CloseOnPressOutside

        background: Rectangle {
            radius: 6
            color: Theme.panel
            border.color: Theme.border
        }

        contentItem: ListView {
            id: listView

            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: completions.items

            ThinScrollBar.vertical: ThinScrollBar {}

            delegate: Rectangle {
                id: delegateItem

                required property int index
                required property string modelData

                width: ListView.view.width
                height: 26
                radius: 4
                color: index === completions.current ? Theme.selected
                                                     : itemMouse.containsMouse ? Theme.hover
                                                                               : "transparent"

                Text {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    verticalAlignment: Text.AlignVCenter
                    text: delegateItem.modelData
                    elide: Text.ElideRight
                    color: delegateItem.index === completions.current ? Theme.selectedText : Theme.text
                    font.pixelSize: 12
                }

                MouseArea {
                    id: itemMouse

                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: completions.apply(delegateItem.index)
                }
            }
        }
    }

    // A month to pick the date of a field from.
    Controls.Popup {
        id: calendar

        property Item field: null
        property int fieldIndex: -1

        function show(field, index) {
            calendar.field = field
            calendar.fieldIndex = index
            const today = new Date()
            grid.year = today.getFullYear()
            grid.month = today.getMonth()
            root.place(calendar, field)
            calendar.x = field.mapToItem(root, field.width, 0).x - calendar.width
            calendar.open()
        }

        function step(delta) {
            let month = grid.month + delta
            let year = grid.year
            if (month < 0) {
                month = 11
                year -= 1
            } else if (month > 11) {
                month = 0
                year += 1
            }
            grid.year = year
            grid.month = month
        }

        width: 250
        height: calendarColumn.implicitHeight + 24
        padding: 12
        margins: 0
        closePolicy: Controls.Popup.CloseOnPressOutside | Controls.Popup.CloseOnEscape

        background: Rectangle {
            radius: 8
            color: Theme.panel
            border.color: Theme.border
        }

        contentItem: ColumnLayout {
            id: calendarColumn

            spacing: 6

            RowLayout {
                Layout.fillWidth: true

                ActionButton {
                    compact: true
                    implicitWidth: 26
                    implicitHeight: 26
                    icon: "arrow-left"
                    tip: qsTr("Previous month")
                    onClicked: calendar.step(-1)
                }

                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: grid.title
                    color: Theme.text
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }

                ActionButton {
                    compact: true
                    implicitWidth: 26
                    implicitHeight: 26
                    icon: "arrow-right"
                    tip: qsTr("Next month")
                    onClicked: calendar.step(1)
                }
            }

            Controls.DayOfWeekRow {
                Layout.fillWidth: true
                locale: grid.locale

                delegate: Text {
                    required property string shortName

                    horizontalAlignment: Text.AlignHCenter
                    text: shortName
                    color: Theme.textMuted
                    font.pixelSize: 11
                }
            }

            Controls.MonthGrid {
                id: grid

                // The cells take the size of the grid.
                Layout.fillWidth: true
                Layout.preferredHeight: 6 * 26 + 5 * spacing
                spacing: 2

                delegate: Rectangle {
                    id: cell

                    required property int year
                    required property int month
                    required property int day
                    required property bool today

                    implicitHeight: 26
                    radius: 4
                    color: dayMouse.pressed ? Theme.pressed
                                            : dayMouse.containsMouse ? Theme.hover : "transparent"
                    border.color: today ? Theme.accent : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: cell.day
                        color: cell.month === grid.month ? Theme.text : Theme.textDisabled
                        font.pixelSize: 12
                    }

                    MouseArea {
                        id: dayMouse

                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            const text = search.formatDate(cell.year, cell.month + 1, cell.day)
                            calendar.field.text = text
                            search.setAdvancedValue(calendar.fieldIndex, text)
                            calendar.close()
                        }
                    }
                }
            }
        }
    }
}
