#include "killswitch.h"


#include <QApplication>
#include <QHostAddress>
#include <QRegularExpression>
#include <algorithm>

#include "version.h"

static bool isValidIpOrCidr(const QString &value) {
    static const QRegularExpression re(
        QStringLiteral(R"(^(\d{1,3}\.){3}\d{1,3}(/\d{1,2})?$)"));
    if (!re.match(value).hasMatch()) return false;
    const QStringList ipParts = value.split(QLatin1Char('/'))[0].split(QLatin1Char('.'));
    for (const QString &part : ipParts) {
        bool ok;
        int octet = part.toInt(&ok);
        if (!ok || octet < 0 || octet > 255) return false;
    }
    if (value.contains(QLatin1Char('/'))) {
        bool ok;
        int prefix = value.split(QLatin1Char('/'))[1].toInt(&ok);
        if (!ok || prefix < 0 || prefix > 32) return false;
    }
    return true;
}

#ifdef Q_OS_WIN
    #include "../client/platforms/windows/daemon/windowsfirewall.h"
    #include "../client/platforms/windows/daemon/windowsdaemon.h"
#endif

#ifdef Q_OS_LINUX
    #include "../client/platforms/linux/daemon/linuxfirewall.h"
#endif

#ifdef Q_OS_MACOS
    #include "../client/platforms/macos/daemon/macosfirewall.h"
#endif

KillSwitch* s_instance = nullptr;

KillSwitch* KillSwitch::instance()
{
    if (s_instance == nullptr) {
        s_instance = new KillSwitch(qApp);
    }
    return s_instance;
}

bool KillSwitch::init()
{
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    m_appSettigns = QSharedPointer<SecureQSettings>(new SecureQSettings(ORGANIZATION_NAME, APPLICATION_NAME, nullptr));
#endif

    if (isStrictKillSwitchEnabled()) {
        return disableAllTraffic();
    }

    return true;
}

bool KillSwitch::refresh(bool enabled)
{
#ifdef Q_OS_WIN
    QSettings RegHLM("HKEY_LOCAL_MACHINE\\Software\\" + QString(ORGANIZATION_NAME)
                             + "\\" + QString(APPLICATION_NAME), QSettings::NativeFormat);
    RegHLM.setValue("strictKillSwitchEnabled", enabled);
#endif

#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    m_appSettigns->setValue("Conf/strictKillSwitchEnabled", enabled);
#endif

    if (isStrictKillSwitchEnabled()) {
        return disableAllTraffic();
    }  else {
        return disableKillSwitch();
    }

}

bool KillSwitch::isStrictKillSwitchEnabled()
{
#ifdef Q_OS_WIN
    QSettings RegHLM("HKEY_LOCAL_MACHINE\\Software\\" + QString(ORGANIZATION_NAME)
                             + "\\" + QString(APPLICATION_NAME), QSettings::NativeFormat);
    return RegHLM.value("strictKillSwitchEnabled", false).toBool();
#endif
    return m_appSettigns->value("Conf/strictKillSwitchEnabled", false).toBool();
}

bool KillSwitch::disableKillSwitch() {
#ifdef Q_OS_LINUX
    if (isStrictKillSwitchEnabled()) {
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("000.allowLoopback"), true);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("100.blockAll"), true);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("110.allowNets"), false);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("120.blockNets"), false);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("200.allowVPN"), false);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv6, QStringLiteral("250.blockIPv6"), true);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("290.allowDHCP"), false);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("300.allowLAN"), false);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("310.blockDNS"), false);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("320.allowDNS"), false);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("400.allowPIA"), false);
    } else {
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("000.allowLoopback"), true);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("100.blockAll"), false);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("110.allowNets"), false);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("120.blockNets"), false);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("200.allowVPN"), false);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv6, QStringLiteral("250.blockIPv6"), false);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("290.allowDHCP"), true);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("300.allowLAN"), true);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("310.blockDNS"), false);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("320.allowDNS"), true);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("400.allowPIA"), false);
        LinuxFirewall::uninstall();
    }
#endif

#ifdef Q_OS_MACOS
    if (isStrictKillSwitchEnabled()) {
        MacOSFirewall::setAnchorEnabled(QStringLiteral("000.allowLoopback"), true);
        MacOSFirewall::setAnchorEnabled(QStringLiteral("100.blockAll"), true);
        MacOSFirewall::setAnchorEnabled(QStringLiteral("110.allowNets"), false);
        MacOSFirewall::setAnchorEnabled(QStringLiteral("120.blockNets"), false);
        MacOSFirewall::setAnchorEnabled(QStringLiteral("200.allowVPN"), false);
        MacOSFirewall::setAnchorEnabled(QStringLiteral("250.blockIPv6"), true);
        MacOSFirewall::setAnchorEnabled(QStringLiteral("290.allowDHCP"), false);
        MacOSFirewall::setAnchorEnabled(QStringLiteral("300.allowLAN"), false);
        MacOSFirewall::setAnchorEnabled(QStringLiteral("310.blockDNS"), false);
        MacOSFirewall::setAnchorEnabled(QStringLiteral("400.allowPIA"), false);
    } else {
        MacOSFirewall::uninstall();
    }
#endif

#ifdef Q_OS_WIN
    if (isStrictKillSwitchEnabled()) {
        return disableAllTraffic();
    }
    return WindowsFirewall::create(this)->allowAllTraffic();
#endif

    m_allowedRanges.clear();
    return true;
}

bool KillSwitch::disableAllTraffic() {
#ifdef Q_OS_WIN
    WindowsFirewall::create(this)->enableInterface(-1);
#endif
#ifdef Q_OS_LINUX
    if (!LinuxFirewall::isInstalled()) {
        LinuxFirewall::install();
    }
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("100.blockAll"), true);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("000.allowLoopback"), true);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv6, QStringLiteral("250.blockIPv6"), true);
#endif
#ifdef Q_OS_MACOS
    // double-check + ensure our firewall is installed and enabled. This is necessary as
    // other software may disable pfctl before re-enabling with their own rules (e.g other VPNs)
    if (!MacOSFirewall::isInstalled())
        MacOSFirewall::install();
    MacOSFirewall::ensureRootAnchorPriority();
    MacOSFirewall::setAnchorEnabled(QStringLiteral("100.blockAll"), true);
    MacOSFirewall::setAnchorEnabled(QStringLiteral("000.allowLoopback"), true);
    MacOSFirewall::setAnchorEnabled(QStringLiteral("250.blockIPv6"), true);
#endif
    m_allowedRanges.clear();
    return true;
}

bool KillSwitch::resetAllowedRange(const QStringList &ranges) {

    if (!std::all_of(ranges.cbegin(), ranges.cend(), isValidIpOrCidr)) {
        qCritical() << "IPC: invalid IP/CIDR in ranges, rejecting resetAllowedRange";
        return false;
    }
    m_allowedRanges = ranges;

#ifdef Q_OS_LINUX
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("110.allowNets"), true);
    LinuxFirewall::updateAllowNets(m_allowedRanges);
#endif

#ifdef Q_OS_MACOS
    MacOSFirewall::setAnchorEnabled(QStringLiteral("110.allowNets"), true);
    MacOSFirewall::setAnchorTable(QStringLiteral("110.allowNets"), true, QStringLiteral("allownets"), m_allowedRanges);
#endif

#ifdef Q_OS_WIN
    if (isStrictKillSwitchEnabled()) {
        WindowsFirewall::create(this)->enableInterface(-1);
    }
    WindowsFirewall::create(this)->allowTrafficRange(m_allowedRanges);
#endif

    return true;
}

bool KillSwitch::addAllowedRange(const QStringList &ranges) {
    if (!std::all_of(ranges.cbegin(), ranges.cend(), isValidIpOrCidr)) {
        qCritical() << "IPC: invalid IP/CIDR in ranges, rejecting addAllowedRange";
        return false;
    }
    for (const QString &range : ranges) {
        if (!range.isEmpty() && !m_allowedRanges.contains(range)) {
            m_allowedRanges.append(range);
        }
    }

    return resetAllowedRange(m_allowedRanges);
}
