#include "containerUtils.h"

#include <QMetaEnum>
#include <QObject>
#include <QJsonDocument>

using namespace amnezia;

DockerContainer ContainerUtils::containerFromString(const QString &container)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<DockerContainer>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        DockerContainer c = static_cast<DockerContainer>(i);
        if (container == containerToString(c))
            return c;
    }
    return DockerContainer::None;
}

QString ContainerUtils::containerToString(DockerContainer c)
{
    if (c == DockerContainer::None)
        return "none";
    if (c == DockerContainer::Cloak)
        return "amnezia-openvpn-cloak";
    if (c == DockerContainer::Awg)
        return "amnezia-awg";
    if (c == DockerContainer::Awg2)
        return "amnezia-awg2";
    QMetaEnum metaEnum = QMetaEnum::fromType<DockerContainer>();
    QString containerKey = metaEnum.valueToKey(static_cast<int>(c));

    return "amnezia-" + containerKey.toLower();
}

QString ContainerUtils::containerTypeToString(DockerContainer c)
{
    if (c == DockerContainer::None)
        return "none";
    if (c == DockerContainer::Ipsec)
        return "ikev2";
    if (c == DockerContainer::Awg)
        return "awg";
    if (c == DockerContainer::Awg2)
        return "awg";
    QMetaEnum metaEnum = QMetaEnum::fromType<DockerContainer>();
    QString containerKey = metaEnum.valueToKey(static_cast<int>(c));

    return containerKey.toLower();
}

QList<DockerContainer> ContainerUtils::allContainers()
{
    QMetaEnum metaEnum = QMetaEnum::fromType<DockerContainer>();
    QList<DockerContainer> all;
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        all.append(static_cast<DockerContainer>(i));
    }

    return all;
}

QMap<DockerContainer, QString> ContainerUtils::containerHumanNames()
{
    return { { DockerContainer::None, "Not installed" },
             { DockerContainer::OpenVpn, "OpenVPN" },
             { DockerContainer::ShadowSocks, "OpenVPN over SS" },
             { DockerContainer::Cloak, "OpenVPN over Cloak" },
             { DockerContainer::WireGuard, "WireGuard" },
             { DockerContainer::Awg, "AmneziaWG" },
             { DockerContainer::Awg2, "AmneziaWG" },
             { DockerContainer::Xray, "XRay" },
             { DockerContainer::Ipsec, QObject::tr("IPsec") },
             { DockerContainer::SSXray, "Shadowsocks"},

             { DockerContainer::TorWebSite, QObject::tr("Website in Tor network") },
             { DockerContainer::Dns, QObject::tr("AmneziaDNS") },
             { DockerContainer::Sftp, QObject::tr("SFTP file sharing service") },
             { DockerContainer::Socks5Proxy, QObject::tr("SOCKS5 proxy server") },
             { DockerContainer::MtProxy, QObject::tr("MTProxy (Telegram)") },
             { DockerContainer::Telemt, QObject::tr("Telemt (Telegram)") },
             { DockerContainer::TProxy, QObject::tr("TProxy (Telegram WEB)") },
    };
}

QMap<DockerContainer, QString> ContainerUtils::containerDescriptions()
{
    return {
             { DockerContainer::OpenVpn,
               QObject::tr("This protocol is no longer supported.") },
             { DockerContainer::ShadowSocks,
               QObject::tr("This protocol is no longer supported.") },
             { DockerContainer::Cloak,
               QObject::tr("This protocol is no longer supported.") },
             { DockerContainer::WireGuard,
               QObject::tr("WireGuard - popular VPN protocol with high performance, high speed and low power "
                           "consumption.") },
             { DockerContainer::Awg,
               QObject::tr("AmneziaWG is a special protocol from Amnezia based on WireGuard. "
                           "It provides high connection speed and ensures stable operation even in the most challenging network conditions.") },
             { DockerContainer::Awg2,
               QObject::tr("AmneziaWG is a special protocol from Amnezia based on WireGuard. "
                           "It provides high connection speed and ensures stable operation even in the most challenging network conditions.") },
             { DockerContainer::Xray,
               QObject::tr("This protocol is no longer supported.") },
             { DockerContainer::Ipsec,
               QObject::tr("This protocol is no longer supported.") },
             { DockerContainer::SSXray,
               QObject::tr("This protocol is no longer supported.") },

             { DockerContainer::TorWebSite, QObject::tr("Deploy a WordPress site on the Tor network in two clicks.") },
             { DockerContainer::Dns,
               QObject::tr("Replace the current DNS server with your own. This will increase your privacy level.") },
             { DockerContainer::Sftp,
               QObject::tr("Create a file vault on your server to securely store and transfer files.") },
             { DockerContainer::Socks5Proxy, ("") },
             { DockerContainer::MtProxy,
               QObject::tr("Telegram MTProto proxy server") },
             { DockerContainer::Telemt,
               QObject::tr("Telegram MTProto proxy (Telemt, Rust)") },
             { DockerContainer::TProxy,
               QObject::tr("Telegram WEB proxy (tproxy-server)") },
    };
}

QMap<DockerContainer, QString> ContainerUtils::containerDetailedDescriptions()
{
    return {
        { DockerContainer::WireGuard,
          QObject::tr("WireGuard is a modern, streamlined VPN protocol offering stable connectivity and excellent performance across all devices. "
                      "It uses fixed encryption settings, delivering lower latency and higher data transfer speeds compared to OpenVPN. "
                      "However, WireGuard is easily identifiable by DPI systems due to its distinctive packet signatures, making it susceptible to blocking.\n"
                      "\nFeatures:\n"
                      "* Available on all AmneziaVPN platforms\n"
                      "* Low power consumption on mobile devices\n"
                      "* Minimal configuration required\n"
                      "* Easily detected by DPI systems (susceptible to blocking)\n"
                      "* Operates over UDP protocol") },
        { DockerContainer::Awg2,
          QObject::tr("AmneziaWG is a modern VPN protocol based on WireGuard, "
                      "combining simplified architecture with high performance across all devices. "
                      "It addresses WireGuard's main vulnerability (easy detection by DPI systems) through advanced obfuscation techniques, "
                      "making VPN traffic indistinguishable from regular internet traffic.\n"
                      "\nAmneziaWG is an excellent choice for those seeking a fast, stealthy VPN connection.\n"
                      "\nFeatures:\n"
                      "* Available on all AmneziaVPN platforms\n"
                      "* Low battery consumption on mobile devices\n"
                      "* Minimal settings required\n"
                      "* Undetectable by traffic analysis systems (DPI)\n"
                      "* Operates over UDP protocol") },

        { DockerContainer::TorWebSite, QObject::tr("Website in Tor network") },
        { DockerContainer::Dns, QObject::tr("DNS Service") },
        { DockerContainer::Sftp,
          QObject::tr("After installation, Amnezia will create a\n\n file storage on your server. "
                      "You will be able to access it using\n FileZilla or other SFTP clients, "
                      "as well as mount the disk on your device to access\n it directly from your device.\n\n"
                      "For more detailed information, you can\n find it in the support section under \"Create SFTP file storage.\" ") },
        { DockerContainer::Socks5Proxy, QObject::tr("SOCKS5 proxy server") },
        { DockerContainer::MtProxy,
          QObject::tr("Telegram MTProto proxy server. "
                      "Allows Telegram clients to connect through your server "
                      "using the MTProto protocol. Supports FakeTLS mode for "
                      "bypassing DPI-based blocking.") },
        { DockerContainer::Telemt,
          QObject::tr("Telegram MTProto proxy powered by Telemt (Rust). "
                      "Supports secure and TLS fronting modes with optional traffic masking.") },
        { DockerContainer::TProxy,
          QObject::tr("Telegram WEB proxy. Clients connect over HTTPS to a hostname; "
                      "the server relays traffic to official MTProxy. Requires a domain, "
                      "ports 80 and 443, and a WEB-capable Telegram app.") },
    };
}

ServiceType ContainerUtils::containerService(DockerContainer c)
{
    if (isUnsupportedContainer(c)) {
        return ServiceType::Vpn;
    }
    return ProtocolUtils::protocolService(defaultProtocol(c));
}

Proto ContainerUtils::defaultProtocol(DockerContainer c)
{
    switch (c) {
    case DockerContainer::None: return Proto::Unknown;
    // Fork: OpenVPN (incl. Cloak/ShadowSocks), XRay and IKEv2 are removed
    case DockerContainer::OpenVpn:
    case DockerContainer::Cloak:
    case DockerContainer::ShadowSocks:
    case DockerContainer::Xray:
    case DockerContainer::Ipsec:
    case DockerContainer::SSXray: return Proto::Unknown;
    case DockerContainer::WireGuard: return Proto::WireGuard;
    case DockerContainer::Awg2: return Proto::Awg;
    case DockerContainer::Awg: return Proto::Awg;

    case DockerContainer::TorWebSite: return Proto::TorWebSite;
    case DockerContainer::Dns: return Proto::Dns;
    case DockerContainer::Sftp: return Proto::Sftp;
    case DockerContainer::Socks5Proxy: return Proto::Socks5Proxy;
    case DockerContainer::MtProxy: return Proto::MtProxy;
    case DockerContainer::Telemt: return Proto::Telemt;
    case DockerContainer::TProxy: return Proto::TProxy;
    default: return Proto::Unknown;
    }
}

QString ContainerUtils::containerTypeToProtocolString(DockerContainer c)
{
    if (c == DockerContainer::None)
        return "none";

    Proto p = defaultProtocol(c);
    return ProtocolUtils::protoToString(p);
}

bool ContainerUtils::isSupportedByCurrentPlatform(DockerContainer c)
{
#ifdef Q_OS_WINDOWS
    return true;

#elif defined(Q_OS_IOS)
    // Standard iOS build (without Network Extension limitations)
    switch (c) {
    case DockerContainer::WireGuard: return true;
    case DockerContainer::Awg2: return true;
    case DockerContainer::Awg: return true;
    case DockerContainer::MtProxy: return true;
    case DockerContainer::Telemt: return true;
    case DockerContainer::TProxy: return true;
    default:
        return false;
    }

#elif defined(MACOS_NE)
    // macOS build using Network Extension
    switch (c) {
    case DockerContainer::WireGuard: return true;
    case DockerContainer::Awg2: return true;
    case DockerContainer::Awg: return true;
    case DockerContainer::MtProxy: return true;
    case DockerContainer::Telemt: return true;
    case DockerContainer::TProxy: return true;
    default:
        return false;
    }
#elif defined(Q_OS_MAC)
    return true;

#elif defined(Q_OS_ANDROID)
    switch (c) {
    case DockerContainer::WireGuard: return true;
    case DockerContainer::Awg2: return true;
    case DockerContainer::Awg: return true;
    case DockerContainer::MtProxy: return true;
    case DockerContainer::Telemt: return true;
    case DockerContainer::TProxy: return true;
    default: return false;
    }

#elif defined(Q_OS_LINUX)
    return true;

#else
    return false;
#endif
}

QStringList ContainerUtils::fixedPortsForContainer(DockerContainer c)
{
    Q_UNUSED(c);
    return {};
}

bool ContainerUtils::isEasySetupContainer(DockerContainer container)
{
    switch (container) {
    case DockerContainer::Awg2: return true;
    default: return false;
    }
}

QString ContainerUtils::easySetupHeader(DockerContainer container)
{
    switch (container) {
    case DockerContainer::Awg2: return QObject::tr("Automatic");
    default: return "";
    }
}

QString ContainerUtils::easySetupDescription(DockerContainer container)
{
    switch (container) {
    case DockerContainer::Awg2: return QObject::tr("AmneziaWG protocol will be installed. "
                                         "It provides high connection speed and ensures stable operation even in the most challenging network conditions.");
    default: return "";
    }
}

int ContainerUtils::easySetupOrder(DockerContainer container)
{
    switch (container) {
    case DockerContainer::Awg2: return 1;
    default: return 0;
    }
}

bool ContainerUtils::isShareable(DockerContainer container)
{
    if (isUnsupportedContainer(container)) {
        return false;
    }

    switch (container) {
    case DockerContainer::TorWebSite: return false;
    case DockerContainer::Dns: return false;
    case DockerContainer::Sftp: return false;
    case DockerContainer::Socks5Proxy: return false;
    case DockerContainer::MtProxy: return false;
    case DockerContainer::Telemt: return false;
    case DockerContainer::TProxy: return false;
    default: return true;
    }
}

bool ContainerUtils::isAwgContainer(DockerContainer container)
{
    return container == DockerContainer::Awg || container == DockerContainer::Awg2;
}

bool ContainerUtils::isUnsupportedContainer(DockerContainer container)
{
    // Fork: only AmneziaWG and WireGuard remain; stored servers with these containers can't connect
    switch (container) {
    case DockerContainer::OpenVpn:
    case DockerContainer::Cloak:
    case DockerContainer::ShadowSocks:
    case DockerContainer::Ipsec:
    case DockerContainer::Xray:
    case DockerContainer::SSXray: return true;
    default: return false;
    }
}

QJsonObject ContainerUtils::getProtocolConfigFromContainer(const Proto protocol, const QJsonObject &containerConfig)
{
    QString protocolConfigString = containerConfig.value(ProtocolUtils::protoToString(protocol))
    .toObject()
            .value(configKey::lastConfig)
            .toString();

    return QJsonDocument::fromJson(protocolConfigString.toUtf8()).object();
}

int ContainerUtils::installPageOrder(DockerContainer container)
{
    switch (container) {
    case DockerContainer::WireGuard: return 2;
    case DockerContainer::Awg2: return 1;
    case DockerContainer::MtProxy:
    case DockerContainer::Telemt:
    case DockerContainer::TProxy:
        return 20;
    default: return 0;
    }
}

