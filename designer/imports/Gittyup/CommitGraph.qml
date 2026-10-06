import QtQuick

// A stand-in for the CommitGraph of the "Gittyup" module, which the
// application draws in C++ (src/ui/qml/CommitGraphItem.cpp). It draws
// nothing.
Item {
    property var columns: []
    property var colors: []
    property real laneWidth: 0
    property string initials: ""
    property bool merge: false
    property bool status: false
    property color statusColor: "transparent"
    property color textColor: "transparent"
}
