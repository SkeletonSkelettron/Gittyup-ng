import QtQuick
import QtQuick.Controls.Basic as Controls
import QtQuick.Layouts
import Gittyup

// 'dialog' is the C++ PluginsDialog.
DialogPage {
    // Keep in sync with Plugin::OptionKind.
    readonly property int booleanOption: 0
    readonly property int integerOption: 1
    readonly property int stringOption: 2
    readonly property int listOption: 3

    title: qsTr("Plugins")
    subtitle: qsTr("Plugins check the changes and report their findings in the diff.")
    contentWidth: 640
    acceptText: qsTr("Done")
    rejectVisible: false

    Text {
        visible: dialog.plugins.length === 0
        Layout.fillWidth: true
        text: qsTr("No plugins are installed.")
        color: Theme.textMuted
        font.pixelSize: 13
    }

    Controls.ScrollView {
        id: scroll

        Layout.fillWidth: true
        implicitHeight: Math.min(pluginColumn.implicitHeight, 440)
        visible: dialog.plugins.length > 0
        clip: true

        Controls.ScrollBar.vertical: ThinScrollBar {
            parent: scroll
            x: scroll.width - width
            height: scroll.height
        }

        ColumnLayout {
            id: pluginColumn

            width: scroll.width - 12
            spacing: 12

            Repeater {
                model: dialog.plugins

                delegate: Rectangle {
                    id: plugin

                    required property int index
                    required property var modelData

                    Layout.fillWidth: true
                    implicitHeight: pluginContent.implicitHeight + 24
                    radius: 8
                    color: Theme.base
                    border.color: Theme.border

                    ColumnLayout {
                        id: pluginContent

                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 12
                        spacing: 10

                        RowLayout {
                            spacing: 8

                            Icon {
                                name: "plug"
                                size: 16
                                color: plugin.modelData.valid ? Theme.accent : Theme.deleted
                            }

                            Text {
                                text: plugin.modelData.name
                                color: Theme.text
                                font.pixelSize: 14
                                font.weight: Font.DemiBold
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            visible: !plugin.modelData.valid
                            text: plugin.modelData.error
                            wrapMode: Text.Wrap
                            color: Theme.deleted
                            font.pixelSize: 12
                        }

                        // Options.
                        Repeater {
                            model: plugin.modelData.options

                            delegate: RowLayout {
                                id: option

                                required property var modelData

                                Layout.fillWidth: true
                                spacing: 12

                                Text {
                                    Layout.preferredWidth: 180
                                    text: option.modelData.text
                                    wrapMode: Text.Wrap
                                    color: Theme.text
                                    font.pixelSize: 13
                                }

                                CheckBox {
                                    visible: option.modelData.kind === booleanOption
                                    checked: option.modelData.value === true
                                    onToggled: dialog.setOption(plugin.index, option.modelData.key, checked)
                                }

                                SpinBox {
                                    visible: option.modelData.kind === integerOption
                                    from: 0
                                    to: 999
                                    value: option.modelData.kind === integerOption ? option.modelData.value : 0
                                    onValueModified: dialog.setOption(plugin.index, option.modelData.key, value)
                                }

                                TextField {
                                    Layout.fillWidth: true
                                    visible: option.modelData.kind === stringOption
                                    text: option.modelData.kind === stringOption ? option.modelData.value : ""
                                    onTextEdited: dialog.setOption(plugin.index, option.modelData.key, text)
                                }

                                ComboBox {
                                    Layout.fillWidth: true
                                    visible: option.modelData.kind === listOption
                                    model: option.modelData.opts
                                    currentIndex: option.modelData.kind === listOption ? option.modelData.value - 1 : -1
                                    onActivated: (index) => dialog.setOption(plugin.index, option.modelData.key, index + 1)
                                }

                                Item { Layout.fillWidth: option.modelData.kind !== stringOption && option.modelData.kind !== listOption }
                            }
                        }

                        // Diagnostics.
                        Repeater {
                            model: plugin.modelData.diagnostics

                            delegate: RowLayout {
                                id: diagnostic

                                required property var modelData

                                Layout.fillWidth: true
                                spacing: 10

                                CheckBox {
                                    checked: diagnostic.modelData.enabled
                                    onToggled: dialog.setDiagnosticEnabled(plugin.index, diagnostic.modelData.key, checked)
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 1

                                    Text {
                                        Layout.fillWidth: true
                                        text: diagnostic.modelData.name
                                        elide: Text.ElideRight
                                        color: Theme.text
                                        font.pixelSize: 13
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        visible: text !== ""
                                        text: diagnostic.modelData.description
                                        wrapMode: Text.Wrap
                                        color: Theme.textMuted
                                        font.pixelSize: 11
                                    }
                                }

                                ComboBox {
                                    Layout.preferredWidth: 120
                                    model: [qsTr("Note"), qsTr("Warning"), qsTr("Error")]
                                    currentIndex: diagnostic.modelData.kind
                                    onActivated: (index) => dialog.setDiagnosticKind(plugin.index, diagnostic.modelData.key, index)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
