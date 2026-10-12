#ifndef COORDINATORCONTROLLER_H
#define COORDINATORCONTROLLER_H

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QTimer>

#include "core/controllers/coordinator/coordinatorApi.h"
#include "core/protocols/vpnProtocol.h"

class SecureServersRepository;
class SecureAppSettingsRepository;
class ImportController;
class ConnectionController;

// Keeps servers that came from the VPN coordinator in sync with it (spec section 9.3).
//
// Persistent state lives in SecureAppSettingsRepository (coordinator* accessors):
//   - base URL, device token (encrypted), user { tgId, username };
//   - managed configs: JSON object keyed by storageServerId -> { configId, name, revision, state }.
// Servers themselves are ordinary Native AWG servers in SecureServersRepository; the managed map is the only link.
class CoordinatorController : public QObject
{
    Q_OBJECT

public:
    // Config states reported by the coordinator plus client-side ones.
    // "ok" | "outdated" | "suspended" | "revoked"; "unknown" before the first check.
    explicit CoordinatorController(SecureServersRepository *serversRepository, SecureAppSettingsRepository *appSettingsRepository,
                                   ImportController *importController, ConnectionController *connectionController,
                                   CoordinatorApi *api, QObject *parent = nullptr);

    bool isLoggedIn() const;
    QString baseUrl() const;
    QString username() const;

    // Account
    void login(const QString &baseUrl, const QString &code);
    void logout(); // revokes the token on the server (best effort) and forgets it locally; managed servers stay as static configs

    // Remote configs of this account (GET /v1/me); result in remoteConfigsUpdated().
    void fetchRemoteConfigs();
    // Create a config on the coordinator and add it to the app.
    void createConfig(const QString &name);
    // Download an existing remote config and add it to the app (no-op re-download if already added).
    void addConfigToApp(const QString &configId);
    // Delete the config on the coordinator and remove its server from the app.
    void deleteConfig(const QString &configId);

    // Check every managed config (status -> fetch if outdated -> replace in place). Silent on network errors.
    void refreshAll();

    // storageServerId of the managed server for configId, or empty.
    QString serverIdForConfig(const QString &configId) const;
    bool isManagedServer(const QString &serverId) const;
    QJsonObject managedConfigs() const;

    // Hook for the app: start the periodic timer and do the startup check.
    void start();

    static constexpr int refreshIntervalMsecs = 15 * 60 * 1000;

signals:
    void loggedInChanged();
    void loginFinished(bool success, const QString &errorText);
    void remoteConfigsUpdated(const QJsonArray &configs); // [{ id, name, revision, createdAt, inApp: bool, state }]
    void configAdded(const QString &serverId);
    void configReplaced(const QString &serverId);
    void managedConfigsChanged();
    // Human-readable problem for the user (not member, suspended, revoked, re-login needed, request failed).
    void errorOccurred(const QString &text);
    void busyChanged(bool busy);
    void networkStateChanged(const QString &network); // "ok" | "degraded" | "down"

private:
    void onConnectionStateChanged(Vpn::ConnectionState state);
    void onServerRemoved(const QString &serverId);

    void checkConfig(const QString &serverId);
    // Import Config JSON (from GET/POST) as a new server or replace serverId's config if non-empty.
    bool applyConfig(const QJsonObject &config, const QString &existingServerId);
    void handleAuthError(const CoordinatorApi::Reply &reply); // 401 -> forget token; 403 not_member -> message
    QString describeError(const CoordinatorApi::Reply &reply) const;
    void setManagedEntry(const QString &serverId, const QJsonObject &entry); // empty entry removes it
    bool setManagedState(const QString &serverId, const QString &state); // true if the stored state changed
    void reconnectIfActive(const QString &serverId);
    void allowThroughKillSwitch();
    QString deviceLabel() const;

    SecureServersRepository *m_serversRepository;
    SecureAppSettingsRepository *m_appSettingsRepository;
    ImportController *m_importController;
    ConnectionController *m_connectionController;
    CoordinatorApi *m_api;

    QTimer m_refreshTimer;
    QString m_lastNetworkState;
    QString m_pendingReconnectServerId;
    qint64 m_lastErrorRefreshMsecs = 0;
    bool m_notMemberReported = false;
};

#endif // COORDINATORCONTROLLER_H
