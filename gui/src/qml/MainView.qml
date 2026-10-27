import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import org.streetpea.chiaking

Pane {
    padding: 0
    id: consolePane
    StackView.onActivated: {
        forceActiveFocus(Qt.TabFocusReason);
        if(!Chiaki.autoConnect && !root.initialAsk && !Chiaki.window.directStream)
        {
            root.initialAsk = true;
            if(Chiaki.settings.addSteamShortcutAsk && (typeof Chiaki.createSteamShortcut === "function"))
                root.showRemindDialog(qsTr("Official Steam artwork + controller layout"), qsTr("Would you like to either create a new non-Steam game for chiaki-ng\nor update an existing non-Steam game with the official artwork and controller layout?") + "\n\n" + qsTr("(Note: If you select no now and want to do this later, click the button or press R3 from the main menu.)"), false, () => root.showSteamShortcutDialog(true));
            else if(Chiaki.settings.remotePlayAsk)
            {
                if(!Chiaki.settings.psnRefreshToken || !Chiaki.settings.psnAuthToken || !Chiaki.settings.psnAuthTokenExpiry || !Chiaki.settings.psnAccountId)
                    root.showRemindDialog(qsTr("Remote Play via PSN"), qsTr("Would you like to connect to PSN?\nThis enables:\n- Automatic registration\n- Playing outside of your home network without port forwarding?") + "\n\n" + qsTr("(Note: If you select no now and want to do this later, go to the Config section of the settings.)"), true, () => root.showPSNTokenDialog(false));
                else
                    Chiaki.settings.remotePlayAsk = false;
            }
        }
    }
    Keys.onUpPressed: {
        if(hostsView.currentItem && hostsView.currentItem.visible)
        {
            hostsView.decrementCurrentIndex()
            while(!hostsView.currentItem.visible)
                hostsView.decrementCurrentIndex()
        }
    }
    Keys.onDownPressed: {
        if(hostsView.currentItem && hostsView.currentItem.visible)
        {
            hostsView.incrementCurrentIndex()
            while(!hostsView.currentItem.visible)
                 hostsView.incrementCurrentIndex()
        }
    }
    Keys.onMenuPressed: settingsButton.clicked()
    Keys.onReturnPressed: if (hostsView.currentItem) hostsView.currentItem.connectToHost()
    Keys.onYesPressed: if (hostsView.currentItem) hostsView.currentItem.wakeUpHost()
    Keys.onNoPressed: if (hostsView.currentItem) hostsView.currentItem.deleteHost()
    Keys.onEscapePressed: root.showConfirmDialog(qsTr("Quit"), qsTr("Are you sure you want to quit?"), () => Qt.quit())
    Keys.onPressed: (event) => {
        if (event.modifiers)
            return;
        switch (event.key) {
        case Qt.Key_PageUp:
            if (hostsView.currentItem) hostsView.currentItem.setConsolePin();
            event.accepted = true;
            break;
        case Qt.Key_PageDown:
            if (Chiaki.settings.psnAuthToken) Chiaki.refreshPsnToken();
            event.accepted = true;
            break;
        case Qt.Key_F1:
            if (typeof Chiaki.createSteamShortcut === "function") root.showSteamShortcutDialog(false);
            event.accepted = true;
            break;
        case Qt.Key_F2:
            root.showManualHostDialog();
            event.accepted = true;
            break;
        }
    }

    background: Item {}

    ToolBar {
        id: toolBar
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        height: 88
        background: Item {
            Rectangle {
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                    leftMargin: root.theme.margin
                    rightMargin: root.theme.margin
                }
                height: 1
                color: root.theme.divider
            }
        }

        RowLayout {
            anchors {
                fill: parent
                leftMargin: 12
                rightMargin: root.theme.margin
                topMargin: 12
                bottomMargin: 8
            }
            spacing: 10

            Button {
                Layout.fillHeight: true
                Layout.preferredWidth: 80
                flat: true
                text: "×"
                font.pixelSize: 52
                focusPolicy: Qt.NoFocus
                onClicked: Qt.quit()
                Material.roundedScale: Material.MediumScale
            }

            Label {
                Layout.leftMargin: 8
                text: "chiaki-ng"
                font.pixelSize: 30
                font.bold: true
                font.letterSpacing: 0.5
                color: root.theme.textPrimary
            }

            Item { Layout.fillWidth: true }

            Button {
                Layout.preferredHeight: 56
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Create Steam Shortcut")
                Material.elevation: 0
                Material.background: root.theme.surfaceHover
                rightPadding: 56
                focusPolicy: Qt.NoFocus
                onClicked: root.showSteamShortcutDialog(false)
                Material.roundedScale: Material.FullScale
                visible: typeof Chiaki.createSteamShortcut === "function"
                Image {
                    anchors {
                        right: parent.right
                        verticalCenter: parent.verticalCenter
                        rightMargin: 16
                    }
                    width: 28
                    height: 28
                    sourceSize: Qt.size(width, height)
                    source: "qrc:/icons/l3.svg"
                }
            }

            Button {
                Layout.preferredHeight: 56
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Refresh PSN Hosts")
                Material.elevation: 0
                Material.background: root.theme.surfaceHover
                icon.source: "qrc:/icons/r1.svg"
                focusPolicy: Qt.NoFocus
                onClicked: Chiaki.refreshPsnToken();
                Material.roundedScale: Material.FullScale
                visible: Chiaki.settings.psnAuthToken
            }

            Button {
                Layout.preferredHeight: 56
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Add Manual Host")
                Material.elevation: 0
                Material.background: root.theme.surfaceHover
                leftPadding: 56
                focusPolicy: Qt.NoFocus
                onClicked: root.showManualHostDialog()
                Material.roundedScale: Material.FullScale
                Image {
                    anchors {
                        left: parent.left
                        verticalCenter: parent.verticalCenter
                        leftMargin: 16
                    }
                    width: 28
                    height: 28
                    sourceSize: Qt.size(width, height)
                    source: "qrc:/icons/r3.svg"
                }
            }

            Button {
                id: settingsButton
                Layout.fillHeight: true
                Layout.preferredWidth: 80
                flat: true
                icon.source: "qrc:/icons/settings-20px.svg";
                icon.width: 40
                icon.height: 40
                focusPolicy: Qt.NoFocus
                onClicked: root.showSettingsDialog()
                Material.roundedScale: Material.MediumScale
            }
        }
    }

    Label {
        id: sectionLabel
        anchors {
            top: toolBar.bottom
            left: hostsView.left
            topMargin: 28
            leftMargin: 4
        }
        text: qsTr("Consoles")
        font.pixelSize: 18
        font.bold: true
        font.letterSpacing: 1
        font.capitalization: Font.AllUppercase
        color: root.theme.textSecondary
    }

    ListView {
        id: hostsView
        keyNavigationWraps: true
        anchors {
            top: sectionLabel.bottom
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
            topMargin: 14
            bottomMargin: 110
        }
        width: Math.min(parent.width - 2 * root.theme.margin, 1280)
        clip: true
        model: Chiaki.hosts
        onCountChanged: {
            if(!hostsView.currentItem)
                hostsView.incrementCurrentIndex();
            if(!hostsView.currentItem)
                return;
            if(!hostsView.currentItem.visible)
            {
                for(var i = 0; i < hostsView.count; i++)
                {
                    hostsView.incrementCurrentIndex()
                    if(hostsView.currentItem.visible)
                    {
                        break;
                    }
                }
            }
        }
        delegate: ItemDelegate {
            visible: modelData.display
            id: delegate
            width: parent ? parent.width : 0
            // the card leaves a gap below it; ListView spacing would also apply to hidden hosts
            height: modelData.display ? 196 : 0
            highlighted: ListView.isCurrentItem
            onClicked: connectToHost()

            function connectToHost() {
                if(modelData.discovered)
                    Chiaki.connectToHost(index, modelData.name);
                else
                    Chiaki.connectToHost(index);
            }

            function wakeUpHost() {
                if(!modelData.discovered && !modelData.duid)
                    Chiaki.wakeUpHost(index);
            }

            function deleteHost() {
                if (modelData.manual)
                    root.showConfirmDialog(qsTr("Delete Console"), qsTr("Are you sure you want to delete this console?"), () => {Chiaki.deleteHost(index)});
                        
                else if (modelData.discovered && !modelData.registered)
                    root.showConfirmDialog(qsTr("Hide Console"), qsTr("Are you sure you want to hide this console?") + "\n\n" + qsTr("Note: You can unhide from the Consoles section of the Settings under Hidden Consoles"), () => Chiaki.hideHost(modelData.mac, modelData.name));

            }

            function setConsolePin() {
                root.showConsolePinDialog(index);
            }

            background: Rectangle {
                anchors {
                    fill: parent
                    bottomMargin: root.theme.spacing
                }
                radius: root.theme.radiusLarge
                color: delegate.highlighted ? root.theme.surfaceSelected : (delegate.hovered ? root.theme.surfaceHover : root.theme.surface)
                border.color: delegate.highlighted ? root.theme.accent : root.theme.border
                border.width: delegate.highlighted ? 2 : 1
                Behavior on color { ColorAnimation { duration: 150 } }
                Behavior on border.color { ColorAnimation { duration: 150 } }
            }

            RowLayout {
                anchors {
                    fill: parent
                    leftMargin: 28
                    rightMargin: 20
                    topMargin: 18
                    bottomMargin: 18 + root.theme.spacing
                }
                spacing: 32

                Rectangle {
                    Layout.fillHeight: true
                    Layout.preferredWidth: 168
                    radius: root.theme.radiusMedium
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, 0.16) }
                        GradientStop { position: 1.0; color: Qt.rgba(1, 1, 1, 0.05) }
                    }

                    Image {
                        anchors {
                            fill: parent
                            margins: 14
                        }
                        fillMode: Image.PreserveAspectFit
                        source: "image://svg/console-ps" + (modelData.ps5 ? "5" : "4") + (modelData.state == "standby" ? "#light_standby" : "#light_on")
                        sourceSize: Qt.size(width, height)
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 6

                    RowLayout {
                        spacing: 16

                        Label {
                            text: modelData.name ? modelData.name : (modelData.ps5 ? "PS5" : "PS4")
                            font.pixelSize: 28
                            font.bold: true
                            color: root.theme.textPrimary
                            elide: Text.ElideRight
                        }

                        Rectangle {
                            visible: !modelData.duid
                            implicitWidth: stateRow.implicitWidth + 24
                            implicitHeight: stateRow.implicitHeight + 10
                            radius: height / 2
                            color: Qt.rgba(1, 1, 1, 0.06)
                            border.color: root.theme.border

                            RowLayout {
                                id: stateRow
                                anchors.centerIn: parent
                                spacing: 8

                                Rectangle {
                                    implicitWidth: 10
                                    implicitHeight: 10
                                    radius: 5
                                    color: root.theme.stateColor(modelData.state)
                                }

                                Label {
                                    text: modelData.state
                                    font.pixelSize: 16
                                    color: root.theme.textPrimary
                                }
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        font.pixelSize: 17
                        color: root.theme.textSecondary
                        text: {
                            let parts = [];
                            if (modelData.address)
                                parts.push(qsTr("Address: %1").arg(Chiaki.settings.streamerMode ? "hidden" : modelData.address));
                            if (modelData.mac)
                                parts.push(qsTr("ID: %1 (%2)").arg(Chiaki.settings.streamerMode ? "hidden" : modelData.mac).arg(modelData.registered ? qsTr("registered") : qsTr("unregistered")));
                            return parts.join("   ·   ");
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        font.pixelSize: 17
                        color: root.theme.textSecondary
                        visible: text.length > 0
                        text: {
                            if (modelData.duid || !modelData.discovered)
                                return "";
                            let parts = [];
                            if (modelData.app)
                                parts.push(qsTr("App: %1").arg(modelData.app));
                            if (modelData.titleId)
                                parts.push(qsTr("Title ID: %1").arg(modelData.titleId));
                            return parts.join("   ·   ");
                        }
                    }

                    Rectangle {
                        Layout.topMargin: 4
                        implicitWidth: connectionLabel.implicitWidth + 20
                        implicitHeight: connectionLabel.implicitHeight + 8
                        radius: root.theme.radiusSmall
                        color: Qt.rgba(0.12, 0.53, 1, 0.14)

                        Label {
                            id: connectionLabel
                            anchors.centerIn: parent
                            font.pixelSize: 15
                            color: root.theme.accent
                            text: {
                                if (modelData.duid)
                                    return modelData.discovered ? qsTr("Automatic Registration Available") : qsTr("Remote Connection via PSN");
                                if (modelData.discovered)
                                    return modelData.manual ? qsTr("discovered + manual") : qsTr("discovered");
                                return qsTr("manual");
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 4

                    Button {
                        Layout.alignment: Qt.AlignRight
                        text: qsTr("Play")
                        padding: 18
                        leftPadding: delegate.highlighted ? 56 : 28
                        rightPadding: 28
                        font.bold: true
                        focusPolicy: Qt.NoFocus
                        onClicked: delegate.connectToHost()
                        Material.background: root.theme.accentStrong
                        Material.foreground: "white"
                        Material.roundedScale: Material.FullScale

                        Image {
                            anchors {
                                left: parent.left
                                verticalCenter: parent.verticalCenter
                                leftMargin: 18
                            }
                            width: 28
                            height: 28
                            sourceSize: Qt.size(width, height)
                            source: root.controllerButton("cross")
                            visible: delegate.highlighted
                        }
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignRight
                        spacing: 4

                        Button {
                            text: modelData.manual ? qsTr("Delete") : qsTr("Hide")
                            Material.elevation: 0
                            Material.background: root.theme.surfaceHover
                            leftPadding: delegate.highlighted ? 52 : 24
                            rightPadding: 24
                            focusPolicy: Qt.NoFocus
                            visible: modelData.manual || (modelData.discovered && !modelData.registered)
                            onClicked: delegate.deleteHost()
                            Material.roundedScale: Material.FullScale

                            Image {
                                anchors {
                                    left: parent.left
                                    verticalCenter: parent.verticalCenter
                                    leftMargin: 12
                                }
                                width: 28
                                height: 28
                                sourceSize: Qt.size(width, height)
                                source: root.controllerButton("box")
                                visible: delegate.highlighted
                            }
                        }

                        Button {
                            text: qsTr("Wake Up")
                            Material.elevation: 0
                            Material.background: root.theme.surfaceHover
                            leftPadding: delegate.highlighted ? 52 : 24
                            rightPadding: 24
                            visible: modelData.registered && !modelData.duid && !modelData.discovered
                            focusPolicy: Qt.NoFocus
                            onClicked: delegate.wakeUpHost()
                            Material.roundedScale: Material.FullScale

                            Image {
                                anchors {
                                    left: parent.left
                                    verticalCenter: parent.verticalCenter
                                    leftMargin: 12
                                }
                                width: 28
                                height: 28
                                sourceSize: Qt.size(width, height)
                                source: root.controllerButton("pyramid")
                                visible: delegate.highlighted
                            }
                        }

                        Button {
                            text: qsTr("Update Console Pin")
                            Material.elevation: 0
                            Material.background: root.theme.surfaceHover
                            leftPadding: delegate.highlighted ? 52 : 24
                            rightPadding: 24
                            visible: modelData.registered
                            focusPolicy: Qt.NoFocus
                            onClicked: delegate.setConsolePin()
                            Material.roundedScale: Material.FullScale

                            Image {
                                anchors {
                                    left: parent.left
                                    verticalCenter: parent.verticalCenter
                                    leftMargin: 12
                                }
                                width: 28
                                height: 28
                                sourceSize: Qt.size(width, height)
                                source: "qrc:/icons/l1.svg"
                                visible: delegate.highlighted
                            }
                        }
                    }
                }
            }
        }
    }

    RoundButton {
        anchors {
            left: parent.left
            bottom: parent.bottom
            margins: 24
        }
        icon.source: "qrc:/icons/discover-" + (checked ? "" : "off-") + "24px.svg"
        icon.width: 30
        icon.height: 30
        padding: 14
        focusPolicy: Qt.NoFocus
        checkable: true
        checked: Chiaki.discoveryEnabled
        onToggled: Chiaki.discoveryEnabled = !Chiaki.discoveryEnabled
        Material.elevation: 0
        Material.background: checked ? root.theme.accentStrong : root.theme.surfaceHover
        ToolTip.visible: hovered
        ToolTip.text: checked ? qsTr("Console discovery on") : qsTr("Console discovery off")
    }

    Label {
        anchors {
            right: parent.right
            bottom: parent.bottom
            margins: 28
        }
        text: Qt.application.version
        font.pixelSize: 16
        color: root.theme.textSecondary
    }

    Image {
        id: logoImage
        anchors.centerIn: parent
        visible: hostsView.count === 0
        source: "qrc:/icons/chiaking-logo-white.svg"
        sourceSize: Qt.size(Math.min(parent.width, parent.height) / 3, Math.min(parent.width, parent.height) / 3)

        PropertyAnimation {
            target: logoImage
            property: "opacity"
            from: 0.05
            to: 0.20
            duration: 1000
            easing.type: Easing.OutCubic
            running: true
        }
    }

    Label {
        anchors {
            top: logoImage.bottom
            horizontalCenter: parent.horizontalCenter
            topMargin: 24
        }
        visible: logoImage.visible
        text: Chiaki.discoveryEnabled ? qsTr("Looking for consoles on your network…") : qsTr("Turn on console discovery or add a console manually")
        font.pixelSize: 20
        color: root.theme.textSecondary
    }
}
