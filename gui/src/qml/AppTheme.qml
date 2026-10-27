import QtQuick

// Visual design tokens shared by all views (instantiated once as root.theme in Main.qml)
QtObject {
    // Background gradient, deep night blue to near black (PS5 home screen inspired)
    readonly property color backgroundTop: "#0c1d3a"
    readonly property color backgroundBottom: "#03060c"

    // Translucent surfaces layered over the background
    readonly property color surface: Qt.rgba(1, 1, 1, 0.05)
    readonly property color surfaceHover: Qt.rgba(1, 1, 1, 0.08)
    readonly property color surfaceSelected: Qt.rgba(0.12, 0.53, 1, 0.09)
    readonly property color border: Qt.rgba(1, 1, 1, 0.08)
    readonly property color divider: Qt.rgba(1, 1, 1, 0.10)

    // PlayStation blue
    readonly property color accent: "#1f87ff"
    readonly property color accentStrong: "#0070d1"

    readonly property color textPrimary: "#f2f5fa"
    readonly property color textSecondary: "#9aa8bf"

    // Console state colors
    readonly property color stateReady: "#3ddc84"
    readonly property color stateStandby: "#f5a623"
    readonly property color stateUnknown: "#7c8799"

    readonly property int radiusLarge: 20
    readonly property int radiusMedium: 14
    readonly property int radiusSmall: 10

    readonly property int spacing: 16
    readonly property int margin: 32

    function stateColor(state) {
        if (state === "ready")
            return stateReady;
        if (state === "standby")
            return stateStandby;
        return stateUnknown;
    }
}
