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

    function stateText(config) {
        if (config.state === "revoked") {
            return qsTr("Отозван")
        }
        if (config.state === "suspended") {
            return qsTr("Приостановлен")
        }
        return config.inApp ? qsTr("В приложении") : qsTr("Не добавлен")
    }

    function askDelete(config) {
        var headerText = qsTr("Удалить конфиг %1?").arg(config.name)
        var descriptionText = qsTr("Конфиг будет удалён на сервере и из приложения.")
        var yesButtonText = qsTr("Удалить")
        var noButtonText = qsTr("Отмена")

        var yesButtonFunction = function() {
            CoordinatorController.deleteConfig(config.id)
        }
        var noButtonFunction = function() {
        }

        showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
    }

    Component.onCompleted: {
        CoordinatorController.fetchRemoteConfigs()
    }

    Connections {
        target: CoordinatorController

        function onConfigAdded(serverId) {
            PageController.showNotificationMessage(qsTr("Конфиг добавлен"))
        }
    }

    ListViewType {
        id: listView

        anchors.fill: parent
        anchors.topMargin: 20 + PageController.safeAreaTopMargin
        anchors.bottomMargin: 24

        model: CoordinatorController.remoteConfigs

        header: ColumnLayout {
            width: listView.width

            BackButtonType {
                id: backButton

                // Opened straight from the "+" tab when logged in: nothing to go back to
                visible: root.StackView.index > 0
            }

            BaseHeaderType {
                id: header

                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: qsTr("Аккаунт VPN")
                descriptionText: CoordinatorController.username
            }

            WarningType {
                Layout.topMargin: 16
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.fillWidth: true

                visible: CoordinatorController.networkState === "degraded" || CoordinatorController.networkState === "down"

                textString: CoordinatorController.networkState === "down" ? qsTr("Сеть недоступна")
                                                                         : qsTr("Сеть работает с перебоями")

                iconPath: "qrc:/images/controls/alert-circle.svg"
            }
        }

        delegate: ColumnLayout {
            width: listView.width

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 6

                text: modelData.name
                descriptionText: root.stateText(modelData)
                rightImageSource: modelData.inApp ? "qrc:/images/controls/trash.svg" : "qrc:/images/controls/plus.svg"

                enabled: !CoordinatorController.busy

                clickedFunction: function() {
                    if (!modelData.inApp && modelData.state !== "revoked") {
                        CoordinatorController.addConfigToApp(modelData.id)
                    } else {
                        root.askDelete(modelData)
                    }
                }
            }

            DividerType {}
        }

        footer: ColumnLayout {
            width: listView.width

            spacing: 0

            TextFieldWithHeaderType {
                id: nameField

                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Новое устройство")
                textField.placeholderText: qsTr("phone")
                textField.maximumLength: 32

                Connections {
                    target: nameField.textField

                    function onTextChanged() {
                        var name = nameField.textField.text
                        if (name === "" || CoordinatorController.isValidConfigName(name)) {
                            nameField.errorText = ""
                        } else {
                            nameField.errorText = qsTr("Только a-z, 0-9 и -, до 32 символов")
                        }
                    }
                }
            }

            BasicButtonType {
                id: createButton

                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: !CoordinatorController.busy && CoordinatorController.isValidConfigName(nameField.textField.text)

                text: qsTr("Создать")

                clickedFunc: function() {
                    CoordinatorController.createConfig(nameField.textField.text)
                    nameField.textField.text = ""
                }
            }

            BasicButtonType {
                id: logoutButton

                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 24
                implicitHeight: 32

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                disabledColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.vibrantRed

                text: qsTr("Выйти")

                clickedFunc: function() {
                    var headerText = qsTr("Выйти из аккаунта?")
                    var descriptionText = qsTr("Добавленные конфиги останутся в приложении, но перестанут обновляться.")
                    var yesButtonText = qsTr("Выйти")
                    var noButtonText = qsTr("Отмена")

                    var yesButtonFunction = function() {
                        CoordinatorController.logout()
                        if (root.StackView.index > 0) {
                            PageController.closePage()
                        } else {
                            PageController.goToPageHome()
                        }
                    }
                    var noButtonFunction = function() {
                    }

                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }
        }
    }
}
