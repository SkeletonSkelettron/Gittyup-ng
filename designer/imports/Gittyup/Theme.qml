pragma Singleton
import QtQuick

// A stand-in for the Theme of the "Gittyup" module, which the application
// makes in C++ (src/ui/qml/QmlTheme.cpp), with the colors of the Kraken Light
// theme (conf/themes/Kraken Light.lua).
QtObject {
    readonly property bool dark: false

    readonly property color base: "#FFFFFF"
    readonly property color alternate: "#F8F9FB"
    readonly property color panel: "#F1F3F6"
    readonly property color toolbar: "#F8F9FB"
    readonly property color sidebar: "#F1F3F6"
    readonly property color field: "#FFFFFF"
    readonly property color border: "#DADFE6"

    readonly property color text: "#1E232B"
    readonly property color textMuted: "#5E6673"
    readonly property color textDisabled: "#AAB1BC"

    readonly property color hover: "#E6E9EE"
    readonly property color pressed: "#D9DEE5"
    readonly property color selected: "#D7E7FB"
    readonly property color selectedText: "#10233F"

    readonly property color accent: "#0E9F84"
    readonly property color accentText: "#FFFFFF"

    readonly property color badge: "#E5484D"
    readonly property color badgeText: "#FFFFFF"
    readonly property color ahead: "#0E9F84"
    readonly property color behind: "#D9720B"
    readonly property color star: "#E5A500"
    readonly property color added: "#1A7F37"
    readonly property color modified: "#D29922"
    readonly property color deleted: "#CF222E"

    readonly property color diffAddition: "#E3F6E8"
    readonly property color diffDeletion: "#FCE8EA"
    readonly property color diffOurs: "#E0F0FF"
    readonly property color diffTheirs: "#F5E6FF"
    readonly property string monoFont: "Monospace"
    readonly property string codeFont: "Monospace"
    readonly property int codeFontSize: 10

    readonly property color tooltip: "#2A2E36"
    readonly property color tooltipText: "#FFFFFF"
}
