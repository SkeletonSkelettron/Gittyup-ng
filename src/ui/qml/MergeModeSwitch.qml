import QtQuick
import Gittyup

// Shows the conflicts of the selected file in the diff or in the merge
// editor. 'detailView' is the C++ DetailView.
SegmentedControl {
    visible: detailView.diff.conflicted
    model: [qsTr("Diff"), qsTr("Merge Editor")]
    currentIndex: detailView.mergeEditor ? 1 : 0
    onActivated: (index) => detailView.mergeEditor = (index === 1)
}
