import QtQuick
import Gittyup

// Drags the reference 'refName' onto another one, like in GitKraken.
// 'refDrop' is the C++ RefDrop of the page. Owners ignore the click that
// ends a drag.
MouseArea {
    id: area

    property string refName
    // The pointer moved far enough to drag since it was pressed.
    property bool dragged: false
    property point pressPoint

    preventStealing: refName !== ""

    onPressed: (event) => {
        dragged = false
        pressPoint = Qt.point(event.x, event.y)
    }

    onPositionChanged: (event) => {
        if (!pressed || refName === "" || !(event.buttons & Qt.LeftButton))
            return

        const p = mapToItem(null, event.x, event.y)
        if (dragged) {
            refDrop.move(p.x, p.y)
        } else if (Math.abs(event.x - pressPoint.x)
                   + Math.abs(event.y - pressPoint.y) > 8) {
            dragged = true
            refDrop.start(refName, p.x, p.y)
        }
    }

    onReleased: {
        if (dragged)
            refDrop.finish()
    }

    onCanceled: {
        if (dragged) {
            dragged = false
            refDrop.cancel()
        }
    }
}
