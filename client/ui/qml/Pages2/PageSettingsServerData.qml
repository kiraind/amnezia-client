import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Components"
import "../Config"

PageType {
    id: root

    signal lastItemTabClickedSignal()

    Connections {
        target: SettingsController
        function onChangeSettingsFinished(finishedMessage) {
            PageController.showNotificationMessage(finishedMessage)
        }
    }

    ListViewType {
        id: listView

        anchors.fill: parent

        model: serverActions

        delegate: ColumnLayout {
            width: listView.width

            LabelWithButtonType {
                Layout.fillWidth: true

                visible: isVisible

                text: title
                descriptionText: description
                textColor: tColor

                clickedFunction: function() {
                    clickedHandler()
                }
            }

            DividerType {
                visible: isVisible
            }
        }
    }

    property list<QtObject> serverActions: [
        remove,
    ]

    QtObject {
        id: remove

        // Fork: a config from the Lesha VPN account is deleted there too, or it would stay behind as a dead device
        readonly property string configId: CoordinatorController.loggedIn
                                           ? CoordinatorController.configIdForServer(ServersUiController.processedServerId) : ""

        property bool isVisible: true
        readonly property string title: configId !== "" ? qsTr("Delete the config of this device")
                                                        : qsTr("Remove server from application")
        readonly property string description: ""
        readonly property var tColor: AmneziaStyle.color.vibrantRed
        readonly property var clickedHandler: function() {
            var headerText = remove.configId !== "" ? qsTr("Delete the config of this device?")
                                                    : qsTr("Do you want to remove the server from application?")
            var descriptionText = remove.configId !== ""
                    ? qsTr("VPN on this device will stop working. To connect it again, tap \"Connect this device\" in your account.")
                    : qsTr("The server will be removed from this app only.")
            var yesButtonText = qsTr("Continue")
            var noButtonText = qsTr("Cancel")

            var yesButtonFunction = function() {
                if (remove.configId !== "") {
                    // The connection is closed by the controller; the main screen follows the removal
                    CoordinatorController.deleteConfig(remove.configId)
                    PageController.goToPageHome()
                } else if (ServersUiController.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                    PageController.showNotificationMessage(qsTr("Cannot remove server during active connection"))
                } else {
                    PageController.showBusyIndicator(true)
                    ServersUiController.removeServer(ServersUiController.processedServerId)
                    PageController.showBusyIndicator(false)
                }
            }
            var noButtonFunction = function() {

            }

            showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
        }
    }
}
