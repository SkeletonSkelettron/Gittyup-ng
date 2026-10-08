import QtQuick
import Gittyup

// Underline the misspelled words of a text field. Make it a child of the
// field so the positions match.
Item {
    id: root

    // A TextInput, a TextEdit or a TextArea.
    property Item field
    readonly property var ranges: detailView.spellCheck.misspelled(field.text,
                                                                   detailView.spellCheck.revision)

    anchors.fill: parent

    Repeater {
        model: root.ranges

        delegate: Canvas {
            id: underline

            required property var modelData

            // Depend on the content size to update after the layout.
            readonly property rect first: {
                root.field.contentWidth
                root.field.contentHeight
                return root.field.positionToRectangle(modelData.start)
            }
            readonly property rect last: {
                root.field.contentWidth
                root.field.contentHeight
                return root.field.positionToRectangle(modelData.start + modelData.length)
            }

            // A wavy line under the word.
            x: first.x
            y: first.y + first.height - 2
            width: Math.max(4, (last.y === first.y ? last.x : root.field.width) - first.x)
            height: 4
            onWidthChanged: requestPaint()
            onPaint: {
                const ctx = getContext("2d")
                ctx.reset()
                ctx.strokeStyle = Theme.deleted
                ctx.lineWidth = 1.2
                ctx.beginPath()
                for (let x = 0; x <= width; x += 2) {
                    const y = (x / 2) % 2 === 0 ? 3 : 1
                    if (x === 0)
                        ctx.moveTo(x, y)
                    else
                        ctx.lineTo(x, y)
                }
                ctx.stroke()
            }
        }
    }
}
