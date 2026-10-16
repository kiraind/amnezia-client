import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QtCore

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    Connections {
        target: ImportController

        function onQrDecodingFinished() {
            if (Qt.platform.os === "ios") {
                PageController.closePage()
            }
            PageController.goToPage(PageEnum.PageSetupWizardViewConfig)
        }
    }

    ListViewType {
        id: listView

        anchors.fill: parent

        header: ColumnLayout {
            width: listView.width

            // Fork: the "more" button with the logs / support tag drawer is removed
            BaseHeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 24 + PageController.safeAreaTopMargin
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: qsTr("Connection")
            }

            ParagraphTextType {
                objectName: "insertKeyLabel"

                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.bottomMargin: 24

                text: qsTr("Sign in with a code from the Telegram bot: the app will get your connection settings and keep them up to date")
            }
        }

        model: variants

        delegate: ColumnLayout {
            width: listView.width

            CardWithIconsType {
                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.bottomMargin: 16

                visible: isVisible

                headerText: title
                bodyText: description

                showRecommendedBadge: featuredAmneziaConnection
                recommendedText: featuredAmneziaConnection ? qsTr("Recommended") : ""

                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                leftImageSource: imageSource

                onClicked: { handler() }

                Keys.onEnterPressed: this.clicked()
                Keys.onReturnPressed: this.clicked()
            }
        }

    }

    // Fork: only the coordinator login; key, file and QR imports are hidden (configs from them don't auto-update)
    property list<QtObject> variants: [
        coordinatorLogin
    ]
    
    QtObject {
        id: coordinatorLogin

        property bool featuredAmneziaConnection: false
        property string title: qsTr("Sign in with a Telegram code")
        property string description: qsTr("Get a code with the /login command in the bot")
        property string imageSource: "qrc:/images/controls/telegram.svg"
        property bool isVisible: true
        property var handler: function() {
            if (CoordinatorController.loggedIn) {
                PageController.goToPage(PageEnum.PageCoordinatorConfigs)
            } else {
                PageController.goToPage(PageEnum.PageCoordinatorLogin)
            }
        }
    }

    QtObject {
        id: fileOpen

        property bool featuredAmneziaConnection: false
        property string title: qsTr("File with connection settings")
        property string description: qsTr("")
        property string imageSource: "qrc:/images/controls/folder-search-2.svg"
        property bool isVisible: true
        property var handler: function() {
            var nameFilter = "Config files (*.vpn *.conf)"
            var fileName = SystemController.getFileName(qsTr("Open config file"), nameFilter)
            if (fileName !== "") {
                if (ImportController.extractConfigFromFile(fileName)) {
                    PageController.goToPage(PageEnum.PageSetupWizardViewConfig)
                }
            }
        }
    }

    QtObject {
        id: qrScan

        property bool featuredAmneziaConnection: false
        property string title: qsTr("QR code")
        property string description: qsTr("")
        property string imageSource: "qrc:/images/controls/scan-line.svg"
        property bool isVisible: SettingsController.isCameraPresent()
        property var handler: function() {
            ImportController.startDecodingQr()
            if (Qt.platform.os === "ios") {
                PageController.goToPage(PageEnum.PageSetupWizardQrReader)
            }
        }
    }
}
