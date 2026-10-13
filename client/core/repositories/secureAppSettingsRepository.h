#ifndef SECUREAPPSETTINGSREPOSITORY_H
#define SECUREAPPSETTINGSREPOSITORY_H

#include <QObject>
#include <QLocale>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QMap>
#include <QVector>
#include <QDateTime>
#include <QByteArray>
#include <QJsonObject>

#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "secureQSettings.h"

using namespace amnezia;

class SecureAppSettingsRepository : public QObject
{
    Q_OBJECT

public:
    explicit SecureAppSettingsRepository(SecureQSettings* settings, QObject *parent = nullptr);

    QLocale getAppLanguage() const;
    void setAppLanguage(QLocale locale);

    bool useAmneziaDns() const;
    void setUseAmneziaDns(bool enabled);
    QStringList getAllowedDnsServers() const;
    void setAllowedDnsServers(const QStringList &servers);
    QString primaryDns() const;
    void setPrimaryDns(const QString &dns);
    QString secondaryDns() const;
    void setSecondaryDns(const QString &dns);

    RouteMode routeMode() const;
    void setRouteMode(RouteMode mode);
    bool addVpnSite(RouteMode mode, const QString &site, const QStringList &ips = {});
    void addVpnSites(RouteMode mode, const QMap<QString, QStringList> &sites);
    void removeVpnSite(RouteMode mode, const QString &site);
    void removeAllVpnSites(RouteMode mode);
    QVariantMap vpnSites(RouteMode mode) const;

    // Normalizes a stored vpn site value into a list of IPs.
    // Supports both the legacy format (a single IP string) and the current one (a list of IPs).
    static QStringList siteIpList(const QVariant &value);
    bool isSitesSplitTunnelingEnabled() const;
    void setSitesSplitTunnelingEnabled(bool enabled);

    AppsRouteMode appsRouteMode() const;
    void setAppsRouteMode(AppsRouteMode mode);
    void setVpnApps(AppsRouteMode mode, const QVector<InstalledAppInfo> &apps);
    QVector<InstalledAppInfo> vpnApps(AppsRouteMode mode) const;
    bool isAppsSplitTunnelingEnabled() const;
    void setAppsSplitTunnelingEnabled(bool enabled);

    bool isKillSwitchEnabled() const;
    void setKillSwitchEnabled(bool enabled);
    bool isStrictKillSwitchEnabled() const;
    void setStrictKillSwitchEnabled(bool enabled);
    
    bool isAutoConnect() const;
    void setAutoConnect(bool enabled);
    bool isStartMinimized() const;
    void setStartMinimized(bool enabled);
    bool isScreenshotsEnabled() const;
    void setScreenshotsEnabled(bool enabled);


    bool isSaveLogs() const;
    void setSaveLogs(bool enabled);
    QDateTime getLogEnableDate() const;
    void setLogEnableDate(const QDateTime &date);
    
    QString getInstallationUuid(bool createIfNotExists) const;
    void clearSettings();

    QByteArray xraySavedConfigs() const;
    void setXraySavedConfigs(const QByteArray &data);

    // VPN coordinator account (see CoordinatorController).
    QString coordinatorBaseUrl() const;
    void setCoordinatorBaseUrl(const QString &url);
    QString coordinatorToken() const;
    void setCoordinatorToken(const QString &token);
    QJsonObject coordinatorUser() const;
    void setCoordinatorUser(const QJsonObject &user);
    // Last known IPv4 of the coordinator host: bypasses the tunnel and is the fallback when DNS fails
    QString coordinatorApiAddress() const;
    void setCoordinatorApiAddress(const QString &address);
    // storageServerId -> { configId, name, revision, state }
    QJsonObject coordinatorConfigs() const;
    void setCoordinatorConfigs(const QJsonObject &configs);
    void clearCoordinatorAccount(); // token and user only; managed configs are kept

signals:
    void appLanguageChanged(QLocale locale);
    void allowedDnsServersChanged(const QStringList &servers);
    void sitesChanged(RouteMode mode);
    void appsChanged(AppsRouteMode mode);
    void routeModeChanged(RouteMode mode);
    void appsRouteModeChanged(AppsRouteMode mode);
    void sitesSplitTunnelingEnabledChanged(bool enabled);
    void appsSplitTunnelingEnabledChanged(bool enabled);
    void useAmneziaDnsChanged(bool enabled);
    void saveLogsChanged(bool enabled);
    void screenshotsEnabledChanged(bool enabled);
    void settingsCleared();

private:
    void setVpnSites(RouteMode mode, const QVariantMap &sites);
    void setInstallationUuid(const QString &uuid);
    
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
    void setValue(const QString &key, const QVariant &value);

    SecureQSettings* m_settings;
};

#endif // SECUREAPPSETTINGSREPOSITORY_H
