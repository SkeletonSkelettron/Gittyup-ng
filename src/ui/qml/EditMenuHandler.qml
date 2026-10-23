import QtQuick

// Shows the edit actions of its parent, a TextInput or a TextEdit, on a right
// click: Undo, Redo, Cut, Copy, Paste, Delete and Select All.
TapHandler {
    acceptedButtons: Qt.RightButton
    onTapped: host.showEditMenu(parent)
}
