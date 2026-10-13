import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    property bool showServerField: CoordinatorController.defaultBaseUrl === ""

    function serverUrl() {
        var url = serverField.textField.text.trim()
        return url !== "" ? url : CoordinatorController.defaultBaseUrl
    }

    Connections {
        target: CoordinatorController

        function onLoginFinished(success, errorText) {
            if (success) {
                // Straight to the main screen: add a config for this device unless one is already in the app
                PageController.showBusyIndicator(true)
                CoordinatorController.setupThisDevice()
            } else {
                codeField.errorText = errorText
            }
        }

        function onDeviceSetupFinished(success) {
            PageController.showBusyIndicator(false)
            if (success) {
                PageController.goToPageHome()
            } else {
                // The error is already shown; let the user pick or create a config by hand
                PageController.closePage()
                PageController.goToPage(PageEnum.PageCoordinatorConfigs)
            }
        }
    }

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin

        onFocusChanged: {
            if (activeFocus) {
                flick.contentY = 0
            }
        }
    }

    FlickableType {
        id: flick

        anchors.top: backButton.bottom
        anchors.bottom: loginButton.top
        anchors.left: parent.left
        anchors.right: parent.right

        contentHeight: scrollColumn.implicitHeight + 24

        ColumnLayout {
            id: scrollColumn

            width: flick.width
            spacing: 0

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 24

                headerText: qsTr("Вход по коду")
                descriptionText: qsTr("Напишите боту /login и введите полученный код. Код действует 5 минут.")
            }

            TextFieldWithHeaderType {
                id: codeField

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                headerText: qsTr("Код")
                textField.placeholderText: qsTr("ABC123")
                textField.maximumLength: 6
                textField.inputMethodHints: Qt.ImhUppercaseOnly | Qt.ImhNoPredictiveText | Qt.ImhSensitiveData

                Connections {
                    target: codeField.textField

                    function onTextChanged() {
                        var upper = codeField.textField.text.toUpperCase()
                        if (upper !== codeField.textField.text) {
                            codeField.textField.text = upper
                        }
                        if (codeField.errorText !== "") {
                            codeField.errorText = ""
                        }
                    }
                }
            }

            BasicButtonType {
                id: serverToggle

                Layout.alignment: Qt.AlignLeft
                Layout.leftMargin: 8
                Layout.bottomMargin: 8
                implicitHeight: 32

                visible: CoordinatorController.defaultBaseUrl !== ""

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                disabledColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.goldenApricot

                text: qsTr("Адрес сервера")

                clickedFunc: function() {
                    root.showServerField = !root.showServerField
                }
            }

            TextFieldWithHeaderType {
                id: serverField

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 24

                visible: root.showServerField

                headerText: qsTr("Адрес сервера")
                textField.placeholderText: qsTr("https://vpn.example.com")
                textField.text: CoordinatorController.baseUrl || CoordinatorController.defaultBaseUrl
                textField.inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
            }
        }
    }

    BasicButtonType {
        id: loginButton

        z: 2
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 16 + PageController.safeAreaBottomMargin

        enabled: !CoordinatorController.busy && codeField.textField.text.trim().length === 6

        text: qsTr("Войти")

        clickedFunc: function() {
            codeField.errorText = ""
            var url = root.serverUrl()
            if (url === "") {
                serverField.errorText = qsTr("Укажите адрес сервера")
                root.showServerField = true
                return
            }
            serverField.errorText = ""
            CoordinatorController.login(url, codeField.textField.text.trim().toUpperCase())
        }
    }
}
