import QtQuick

// Shows the edit actions of its parent, a TextInput or a TextEdit, on a right
// click: Undo, Redo, Cut, Copy, Paste, Delete and Select All.
// It takes the press from its parent. A TapHandler lets a TextInput have it
// too, and the TextInput moves its cursor to it and loses its selection.
MouseArea {
    anchors.fill: parent
    acceptedButtons: Qt.RightButton
    onClicked: host.showEditMenu(parent)
}
