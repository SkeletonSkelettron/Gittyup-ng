import QtQuick
import QtQuick.Layouts
import Gittyup

// 'dialog' is the C++ InputDialog.
DialogPage {
    id: page

    title: dialog.title
    subtitle: dialog.text
    acceptText: dialog.acceptText
    acceptEnabled: dialog.acceptable
    // Focus the first empty field, like the password after a known user name.
    initialFocus: {
        for (let i = 0; i < fields.count; ++i) {
            if (dialog.fields[i].value === "")
                return fields.itemAt(i).field
        }
        return fields.count > 0 ? fields.itemAt(0).field : null
    }

    Repeater {
        id: fields

        model: dialog.fields

        delegate: FormField {
            required property int index
            required property var modelData
            readonly property alias field: textField

            label: modelData.label

            TextField {
                id: textField

                Layout.fillWidth: true
                text: modelData.value
                placeholderText: modelData.placeholder
                echoMode: modelData.password ? TextInput.Password : TextInput.Normal
                onTextEdited: dialog.setValue(index, text)
                onActiveFocusChanged: if (activeFocus) selectAll()
            }
        }
    }
}
