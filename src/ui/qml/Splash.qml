import QtQuick

// Shown while the main windows load. The main thread is busy meanwhile, so the
// ring turns with an Animator: those run on the render thread.
// 'theme' is the C++ QmlTheme.
Rectangle {
    id: root

    width: 320
    height: 200
    color: theme.panel
    border.color: theme.border

    Column {
        anchors.centerIn: parent
        spacing: 14

        Item {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 88
            height: 88

            Image {
                anchors.centerIn: parent
                width: 60
                height: 60
                source: "qrc:/Gittyup.iconset/icon_128x128.png"
                sourceSize: Qt.size(128, 128)
                smooth: true
            }

            // A ring with a gap around the icon. It is painted once, before
            // the main thread gets busy.
            Canvas {
                anchors.fill: parent
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.reset()
                    ctx.lineWidth = 3
                    ctx.lineCap = "round"
                    ctx.strokeStyle = theme.accent
                    ctx.beginPath()
                    ctx.arc(width / 2, height / 2, width / 2 - 3, 0, Math.PI * 1.5)
                    ctx.stroke()
                }

                RotationAnimator on rotation {
                    from: 0
                    to: 360
                    duration: 1000
                    loops: Animation.Infinite
                }
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Qt.application.displayName
            color: theme.text
            font.pixelSize: 16
            font.bold: true
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Loading repositories…")
            color: theme.textMuted
            font.pixelSize: 12
        }
    }
}
