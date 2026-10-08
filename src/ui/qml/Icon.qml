import QtQuick
import Gittyup

// A monochrome icon from qrc:/qml/icons, tinted with 'color'.
Item {
    id: root

    property string name
    property color color: Theme.text
    property int size: 18

    implicitWidth: size
    implicitHeight: size

    Image {
        anchors.fill: parent
        // Render at twice the size so icons stay sharp on high DPI screens.
        sourceSize: Qt.size(root.size * 2, root.size * 2)
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
        source: root.name ? "image://icons/" + root.name + "/" + root.color.toString().substring(1) : ""
    }
}
