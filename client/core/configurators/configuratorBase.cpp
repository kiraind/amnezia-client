#include "configuratorBase.h"

#include "core/configurators/ikev2Configurator.h"
#include "core/configurators/openVpnConfigurator.h"
#include "core/configurators/wireguardConfigurator.h"
#include "core/configurators/xrayConfigurator.h"

using namespace amnezia;

ConfiguratorBase::ConfiguratorBase(QObject *parent)
    : QObject { parent }
{
}

QScopedPointer<ConfiguratorBase> ConfiguratorBase::create(Proto protocol)
{
    switch (protocol) {
    case Proto::OpenVpn: return QScopedPointer<ConfiguratorBase>(new OpenVpnConfigurator());
    case Proto::WireGuard:
    case Proto::Awg: return QScopedPointer<ConfiguratorBase>(new WireguardConfigurator());
    case Proto::Ikev2: return QScopedPointer<ConfiguratorBase>(new Ikev2Configurator());
    case Proto::Xray: return QScopedPointer<ConfiguratorBase>(new XrayConfigurator());
    case Proto::SSXray: return QScopedPointer<ConfiguratorBase>(new XrayConfigurator());
    default: return QScopedPointer<ConfiguratorBase>();
    }
}

ProtocolConfig ConfiguratorBase::processConfigWithLocalSettings(const ConnectionSettings &settings,
                                                                 ProtocolConfig protocolConfig)
{
    applyDnsToNativeConfig(settings.dns, protocolConfig);
    return protocolConfig;
}

void ConfiguratorBase::applyDnsToNativeConfig(const DnsSettings &dns, ProtocolConfig &protocolConfig)
{
    QString config = protocolConfig.nativeConfig();
    config.replace("$PRIMARY_DNS", dns.primaryDns);
    config.replace("$SECONDARY_DNS", dns.secondaryDns);
    protocolConfig.setNativeConfig(config);
}
