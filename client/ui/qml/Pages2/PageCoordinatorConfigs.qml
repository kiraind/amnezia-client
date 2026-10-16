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

    // One config per device: once this app has one, nothing new can be created here
    readonly property bool hasConfigInApp: {
        var configs = CoordinatorController.remoteConfigs
        for (var i = 0; i < configs.length; i++) {
            if (configs[i].inApp) {
                return true
            }
        }
        return false
    }

    function stateText(config) {
        if (config.state === "revoked") {
            return qsTr("Отозван")
        }
        if (config.state === "suspended") {
            return qsTr("Приостановлен")
        }
        return config.inApp ? qsTr("Это устройство") : qsTr("Другое устройство")
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

        function onConfigRenamed(configId, name) {
            PageController.showNotificationMessage(qsTr("Устройство переименовано"))
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

                headerText: qsTr("Аккаунт %1").arg(Qt.application.displayName)
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

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 6

                spacing: 0

                LabelWithButtonType {
                    Layout.fillWidth: true

                    text: modelData.name
                    descriptionText: root.stateText(modelData)
                    rightImageSource: "qrc:/images/controls/edit-3.svg"

                    enabled: !CoordinatorController.busy

                    // Another device's config is never added here: two devices with one key knock each other off
                    clickedFunction: function() {
                        configDrawer.config = modelData
                        configDrawer.openTriggered()
                    }
                }

                ImageButtonType {
                    Layout.rightMargin: 16

                    implicitWidth: 40
                    implicitHeight: 40

                    image: "qrc:/images/controls/trash.svg"
                    imageColor: AmneziaStyle.color.paleGray

                    enabled: !CoordinatorController.busy

                    onClicked: function() {
                        root.askDelete(modelData)
                    }
                }
            }

            DividerType {}
        }

        footer: ColumnLayout {
            width: listView.width

            spacing: 0

            BasicButtonType {
                id: setupButton

                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: !root.hasConfigInApp
                enabled: !CoordinatorController.busy

                text: qsTr("Подключить это устройство")

                clickedFunc: function() {
                    CoordinatorController.setupThisDevice()
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

    // Rename a config; the name is what the bot and the coordinator call the device
    DrawerType2 {
        id: configDrawer

        property var config: ({})

        anchors.fill: parent

        expandedStateContent: ColumnLayout {
            id: drawerContent

            readonly property string normalizedName: CoordinatorController.toConfigName(nameField.textField.text)

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            spacing: 0

            // Same sizing as QuestionDrawer: as tall as the content
            onImplicitHeightChanged: {
                configDrawer.expandedHeight = drawerContent.implicitHeight + 16 + PageController.safeAreaBottomMargin
            }

            Connections {
                target: configDrawer

                function onAboutToShow() {
                    nameField.textField.text = configDrawer.config.name || ""
                }
            }

            Header2TextType {
                Layout.fillWidth: true
                Layout.topMargin: 16

                text: qsTr("Переименовать устройство")
            }

            TextFieldWithHeaderType {
                id: nameField

                Layout.fillWidth: true
                Layout.topMargin: 16

                headerText: qsTr("Название устройства")
                textField.maximumLength: 64
                checkEmptyText: true
            }

            // Only when the typed text is not saved as is
            CaptionTextType {
                Layout.fillWidth: true
                Layout.topMargin: 8

                visible: drawerContent.normalizedName !== nameField.textField.text

                color: AmneziaStyle.color.mutedGray
                text: drawerContent.normalizedName === ""
                      ? qsTr("Латинские буквы, цифры и дефисы, до 32 символов")
                      : qsTr("Сохранится как %1").arg(drawerContent.normalizedName)
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 16

                enabled: !CoordinatorController.busy && drawerContent.normalizedName !== ""

                text: qsTr("Сохранить")

                clickedFunc: function() {
                    if (drawerContent.normalizedName !== configDrawer.config.name) {
                        CoordinatorController.renameConfig(configDrawer.config.id, drawerContent.normalizedName)
                    }
                    configDrawer.closeTriggered()
                }
            }
        }
    }
}
