#include "configuratorBase.h"

#include "core/configurators/wireguardConfigurator.h"

using namespace amnezia;

ConfiguratorBase::ConfiguratorBase(QObject *parent)
    : QObject { parent }
{
}

QScopedPointer<ConfiguratorBase> ConfiguratorBase::create(Proto protocol)
{
    switch (protocol) {
    case Proto::WireGuard:
    case Proto::Awg: return QScopedPointer<ConfiguratorBase>(new WireguardConfigurator());
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
