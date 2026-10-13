#include "xrayConfigurator.h"

#include "core/models/protocols/xrayProtocolConfig.h"

namespace {
    // Desktop applies this in XrayProtocol::start(); iOS/Android pass JSON straight to libxray — same fixes here.
    void sanitizeXrayNativeConfig(amnezia::ProtocolConfig &pc)
    {
        const QString before = pc.nativeConfig();
        const QString after = amnezia::sanitizeNativeConfig(before);
        if (after != before) {
            pc.setNativeConfig(after);
        }
    }
} // namespace

XrayConfigurator::XrayConfigurator(QObject *parent)
    : ConfiguratorBase(parent)
{
}

amnezia::ProtocolConfig XrayConfigurator::processConfigWithLocalSettings(const amnezia::ConnectionSettings &settings,
                                                                         amnezia::ProtocolConfig protocolConfig)
{
    applyDnsToNativeConfig(settings.dns, protocolConfig);
    sanitizeXrayNativeConfig(protocolConfig);
    return protocolConfig;
}
