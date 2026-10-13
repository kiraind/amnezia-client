#ifndef PAGE_ENUM_H
#define PAGE_ENUM_H

#include <QObject>
#include <QQmlEngine>

namespace PageLoader
{
    Q_NAMESPACE
    enum class PageEnum {
        PageStart = 0,
        PageHome,
        PageAbout,

        PageSettingsServersList,
        PageSettings,
        PageSettingsServerData,
        PageSettingsServerInfo,
        PageSettingsServerProtocols,
        PageSettingsServerProtocol,
        PageSettingsConnection,
        PageSettingsDns,
        PageSettingsApplication,
        PageSettingsAbout,
        PageSettingsLogging,
        PageSettingsSplitTunneling,
        PageSettingsAppSplitTunneling,
        PageSettingsKillSwitch,
        PageSettingsKillSwitchExceptions,

        PageSetupWizardStart,
        PageSetupWizardConfigSource,
        PageSetupWizardTextKey,
        PageSetupWizardViewConfig,
        PageSetupWizardQrReader,

        PageProtocolOpenVpnSettings,
        PageProtocolXraySettings,
        PageProtocolIKev2Settings,
        PageProtocolRaw,

        PageProtocolWireGuardClientSettings,
        PageProtocolAwgClientSettings,

        PageProtocolXraySnapshots,
        PageProtocolXrayTransportSettings,
        PageProtocolXrayXmuxSettings,
        PageProtocolXrayXPaddingSettings,
        PageProtocolXrayFlowSettings,
        PageProtocolXraySecuritySettings,
        PageProtocolXrayXPaddingBytesSettings,

        PageSettingsLanguage,

        PageCoordinatorLogin,
        PageCoordinatorConfigs,
    };
    Q_ENUM_NS(PageEnum)

    static void declareQmlPageEnum()
    {
        qmlRegisterUncreatableMetaObject(PageLoader::staticMetaObject, "PageEnum", 1, 0, "PageEnum", "Error: only enums");
    }
}

#endif // PAGE_ENUM_H
