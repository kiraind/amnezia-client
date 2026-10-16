#include "coordinatorController.h"

#include <QDateTime>
#include <QDebug>
#include <QSysInfo>
#include <QUrl>

#include "core/controllers/connectionController.h"
#include "core/controllers/coordinator/deviceName.h"
#include "core/controllers/selfhosted/importController.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "core/repositories/secureServersRepository.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/errorStrings.h"
#include "core/utils/networkUtilities.h"
#include "core/utils/serverConfigUtils.h"

#ifdef AMNEZIA_DESKTOP
    #include "core/utils/ipcClient.h"
#endif

using namespace amnezia;

namespace
{
namespace entryKey
{
constexpr QLatin1String configId("configId");
constexpr QLatin1String name("name");
constexpr QLatin1String revision("revision");
constexpr QLatin1String state("state");
constexpr QLatin1String displayName("displayName"); // server name the app last set from the coordinator
} // namespace entryKey

constexpr qint64 errorRefreshThrottleMsecs = 60 * 1000;
} // namespace

CoordinatorController::CoordinatorController(SecureServersRepository *serversRepository,
                                             SecureAppSettingsRepository *appSettingsRepository, ImportController *importController,
                                             ConnectionController *connectionController, CoordinatorApi *api, QObject *parent)
    : QObject(parent),
      m_serversRepository(serversRepository),
      m_appSettingsRepository(appSettingsRepository),
      m_importController(importController),
      m_connectionController(connectionController),
      m_api(api)
{
    m_api->setBaseUrl(m_appSettingsRepository->coordinatorBaseUrl());
    m_api->setFallbackAddress(m_appSettingsRepository->coordinatorApiAddress());
    m_api->setToken(m_appSettingsRepository->coordinatorToken());

    // Remember where the coordinator lives: the tunnel bypasses it, and requests fall back to it when DNS fails
    connect(m_api, &CoordinatorApi::addressLearned, this, [this](const QString &address) {
        m_api->setFallbackAddress(address);
        if (m_appSettingsRepository->coordinatorApiAddress() != address) {
            m_appSettingsRepository->setCoordinatorApiAddress(address);
        }
    });

    m_refreshTimer.setInterval(refreshIntervalMsecs);
    connect(&m_refreshTimer, &QTimer::timeout, this, &CoordinatorController::refreshAll);

    connect(m_serversRepository, &SecureServersRepository::serverRemoved, this,
            [this](const QString &serverId, int) { onServerRemoved(serverId); });
    connect(m_connectionController, &ConnectionController::connectionStateChanged, this,
            &CoordinatorController::onConnectionStateChanged);
}

bool CoordinatorController::isLoggedIn() const
{
    return !m_appSettingsRepository->coordinatorToken().isEmpty() && !m_appSettingsRepository->coordinatorBaseUrl().isEmpty();
}

QString CoordinatorController::baseUrl() const
{
    return m_appSettingsRepository->coordinatorBaseUrl();
}

QString CoordinatorController::username() const
{
    // username may be null for Telegram users without one; tgId is the identity then
    const QJsonObject user = m_appSettingsRepository->coordinatorUser();
    const QString name = user.value("username").toString();
    if (!name.isEmpty()) {
        return QStringLiteral("@") + name;
    }
    const QJsonValue tgId = user.value("tgId");
    const QString id = tgId.isString() ? tgId.toString() : QString::number(tgId.toInteger());
    return tgId.isUndefined() || tgId.isNull() ? QString() : QStringLiteral("Telegram ID %1").arg(id);
}

void CoordinatorController::start()
{
    if (!isLoggedIn()) {
        return;
    }
    m_refreshTimer.start();
    QTimer::singleShot(0, this, &CoordinatorController::refreshAll);
}

void CoordinatorController::login(const QString &baseUrl, const QString &code)
{
    QString url = baseUrl.trimmed();
    while (url.endsWith('/')) {
        url.chop(1);
    }
    const QString normalizedCode = code.trimmed().toUpper();

    m_api->setBaseUrl(url);
    allowThroughKillSwitch();
    emit busyChanged(true);
    m_api->login(normalizedCode, deviceLabel(), [this, url](const CoordinatorApi::Reply &reply) {
        emit busyChanged(false);
        const QString token = reply.body.value("token").toString();
        if (!reply.ok() || token.isEmpty()) {
            m_api->setBaseUrl(m_appSettingsRepository->coordinatorBaseUrl());
            m_api->setFallbackAddress(m_appSettingsRepository->coordinatorApiAddress());
            emit loginFinished(false, describeError(reply));
            return;
        }

        m_appSettingsRepository->setCoordinatorBaseUrl(url);
        m_appSettingsRepository->setCoordinatorToken(token);
        m_appSettingsRepository->setCoordinatorUser(reply.body.value("user").toObject());
        m_api->setToken(token);
        m_notMemberReported = false;

        emit loggedInChanged();
        emit loginFinished(true, QString());
        m_refreshTimer.start();
        fetchRemoteConfigs();
    });
}

void CoordinatorController::logout()
{
    if (isLoggedIn()) {
        allowThroughKillSwitch();
        m_api->logout([](const CoordinatorApi::Reply &reply) {
            if (!reply.ok()) {
                qWarning() << "CoordinatorController::logout(): server-side logout failed:" << reply.httpStatus << reply.errorMessage;
            }
        });
    }
    // The request above already captured the token; it is safe to forget it now.
    m_appSettingsRepository->clearCoordinatorAccount();
    m_api->setToken(QString());
    m_refreshTimer.stop();
    emit loggedInChanged();
}

void CoordinatorController::fetchRemoteConfigs()
{
    if (!isLoggedIn()) {
        return;
    }
    allowThroughKillSwitch();
    emit busyChanged(true);
    m_api->me([this](const CoordinatorApi::Reply &reply) {
        emit busyChanged(false);
        if (!reply.ok()) {
            if (reply.unauthorized() || reply.notMember()) {
                handleAuthError(reply);
            } else {
                emit errorOccurred(describeError(reply));
            }
            return;
        }
        m_notMemberReported = false;

        const QJsonObject user = reply.body.value("user").toObject();
        if (!user.isEmpty()) {
            m_appSettingsRepository->setCoordinatorUser(user);
        }

        const QJsonObject managed = managedConfigs();
        QJsonArray result;
        for (const QJsonValue &value : reply.body.value("configs").toArray()) {
            QJsonObject summary = value.toObject();
            const QString serverId = serverIdForConfig(summary.value("id").toString());
            summary["inApp"] = !serverId.isEmpty();
            if (summary.value("state").toString().isEmpty() && !serverId.isEmpty()) {
                summary["state"] = managed.value(serverId).toObject().value(entryKey::state).toString();
            }
            result.append(summary);
        }
        emit remoteConfigsUpdated(result);
    });
}

void CoordinatorController::createConfig(const QString &name)
{
    if (!isLoggedIn()) {
        return;
    }
    allowThroughKillSwitch();
    emit busyChanged(true);
    m_api->createConfig(name.trimmed(), [this](const CoordinatorApi::Reply &reply) {
        emit busyChanged(false);
        if (reply.ok()) {
            applyConfig(reply.body, QString());
        } else if (reply.unauthorized() || reply.notMember()) {
            handleAuthError(reply);
            return;
        } else {
            emit errorOccurred(describeError(reply));
        }
        fetchRemoteConfigs();
    });
}

void CoordinatorController::addConfigToApp(const QString &configId)
{
    if (!isLoggedIn()) {
        return;
    }
    const QString existingServerId = serverIdForConfig(configId);
    if (!existingServerId.isEmpty()) {
        allowThroughKillSwitch();
        checkConfig(existingServerId);
        return;
    }

    allowThroughKillSwitch();
    emit busyChanged(true);
    m_api->getConfig(configId, [this](const CoordinatorApi::Reply &reply) {
        emit busyChanged(false);
        if (reply.ok()) {
            applyConfig(reply.body, QString());
        } else if (reply.unauthorized() || reply.notMember()) {
            handleAuthError(reply);
            return;
        } else {
            emit errorOccurred(describeError(reply));
        }
        fetchRemoteConfigs();
    });
}

void CoordinatorController::setupThisDevice()
{
    if (!isLoggedIn()) {
        emit deviceSetupFinished(false);
        return;
    }
    if (!managedConfigs().isEmpty()) {
        emit deviceSetupFinished(true);
        return;
    }

    allowThroughKillSwitch();
    emit busyChanged(true);
    m_api->me([this](const CoordinatorApi::Reply &reply) {
        if (!reply.ok()) {
            emit busyChanged(false);
            if (reply.unauthorized() || reply.notMember()) {
                handleAuthError(reply);
            } else {
                emit errorOccurred(describeError(reply));
            }
            emit deviceSetupFinished(false);
            return;
        }

        QStringList takenNames;
        for (const QJsonValue &value : reply.body.value("configs").toArray()) {
            takenNames.append(value.toObject().value("name").toString());
        }
        const QString name = deviceConfigName(deviceName::platformName(), QSysInfo::productType(), takenNames);

        m_api->createConfig(name, [this](const CoordinatorApi::Reply &reply) {
            emit busyChanged(false);
            bool added = false;
            if (reply.ok()) {
                added = applyConfig(reply.body, QString());
            } else if (reply.unauthorized() || reply.notMember()) {
                handleAuthError(reply);
            } else {
                emit errorOccurred(describeError(reply));
            }
            emit deviceSetupFinished(added);
            if (isLoggedIn()) {
                fetchRemoteConfigs();
            }
        });
    });
}

QString CoordinatorController::deviceConfigName(const QString &deviceName, const QString &fallback, const QStringList &takenNames)
{
    // 28 leaves room for a "-NN" suffix
    QString base = deviceName.trimmed().compare(QLatin1String("localhost"), Qt::CaseInsensitive) == 0
            ? QString()
            : deviceName::toConfigName(deviceName, 28);
    if (base.isEmpty()) {
        base = deviceName::toConfigName(fallback, 28);
    }
    if (base.isEmpty()) {
        base = QStringLiteral("device");
    }

    QString name = base;
    for (int i = 2; takenNames.contains(name); ++i) {
        name = QStringLiteral("%1-%2").arg(base).arg(i);
    }
    return name;
}

void CoordinatorController::renameConfig(const QString &configId, const QString &name)
{
    if (!isLoggedIn()) {
        return;
    }
    allowThroughKillSwitch();
    emit busyChanged(true);
    m_api->renameConfig(configId, name, [this, configId](const CoordinatorApi::Reply &reply) {
        emit busyChanged(false);
        if (reply.ok()) {
            const QString newName = reply.body.value("name").toString();
            const QString serverId = serverIdForConfig(configId);
            if (!serverId.isEmpty()) {
                QJsonObject entry = managedConfigs().value(serverId).toObject();
                entry[entryKey::name] = newName;
                setManagedEntry(serverId, entry);
            }
            emit configRenamed(configId, newName);
        } else if (reply.unauthorized() || reply.notMember()) {
            handleAuthError(reply);
            return;
        } else {
            emit errorOccurred(describeError(reply));
        }
        fetchRemoteConfigs();
    });
}

void CoordinatorController::deleteConfig(const QString &configId)
{
    if (!isLoggedIn()) {
        return;
    }
    allowThroughKillSwitch();
    emit busyChanged(true);
    m_api->deleteConfig(configId, [this, configId](const CoordinatorApi::Reply &reply) {
        emit busyChanged(false);
        if (reply.ok() || reply.notFound()) {
            const QString serverId = serverIdForConfig(configId);
            if (!serverId.isEmpty()) {
                if (m_connectionController->isConnected() && m_serversRepository->defaultServerId() == serverId) {
                    m_pendingReconnectServerId.clear();
                    m_connectionController->closeConnection();
                }
                if (m_serversRepository->indexOfServerId(serverId) >= 0) {
                    m_serversRepository->removeServer(serverId); // onServerRemoved() drops the managed entry
                } else {
                    setManagedEntry(serverId, QJsonObject());
                }
            }
        } else if (reply.unauthorized() || reply.notMember()) {
            handleAuthError(reply);
            return;
        } else {
            emit errorOccurred(describeError(reply));
        }
        fetchRemoteConfigs();
    });
}

void CoordinatorController::refreshAll()
{
    if (!isLoggedIn()) {
        return;
    }
    const QStringList serverIds = managedConfigs().keys();
    if (serverIds.isEmpty()) {
        return;
    }
    allowThroughKillSwitch();
    for (const QString &serverId : serverIds) {
        checkConfig(serverId);
    }
}

QString CoordinatorController::serverIdForConfig(const QString &configId) const
{
    if (configId.isEmpty()) {
        return QString();
    }
    const QJsonObject managed = managedConfigs();
    for (auto it = managed.constBegin(); it != managed.constEnd(); ++it) {
        if (it.value().toObject().value(entryKey::configId).toString() == configId) {
            return it.key();
        }
    }
    return QString();
}

bool CoordinatorController::isManagedServer(const QString &serverId) const
{
    return !serverId.isEmpty() && managedConfigs().contains(serverId);
}

QJsonObject CoordinatorController::managedConfigs() const
{
    return m_appSettingsRepository->coordinatorConfigs();
}

void CoordinatorController::onConnectionStateChanged(Vpn::ConnectionState state)
{
    if (state == Vpn::ConnectionState::Disconnected && !m_pendingReconnectServerId.isEmpty()) {
        const QString serverId = m_pendingReconnectServerId;
        m_pendingReconnectServerId.clear();
        if (m_serversRepository->indexOfServerId(serverId) >= 0) {
            const ErrorCode error = m_connectionController->openConnection(serverId);
            if (error != ErrorCode::NoError) {
                qWarning() << "CoordinatorController: reconnect after config update failed:" << errorString(error);
            }
        }
        return;
    }

    if (state == Vpn::ConnectionState::Error && isLoggedIn()) {
        const QString serverId = m_serversRepository->defaultServerId();
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (isManagedServer(serverId) && now - m_lastErrorRefreshMsecs >= errorRefreshThrottleMsecs) {
            m_lastErrorRefreshMsecs = now;
            allowThroughKillSwitch();
            checkConfig(serverId);
        }
    }
}

void CoordinatorController::onServerRemoved(const QString &serverId)
{
    if (isManagedServer(serverId)) {
        setManagedEntry(serverId, QJsonObject());
    }
    if (m_pendingReconnectServerId == serverId) {
        m_pendingReconnectServerId.clear();
    }
}

void CoordinatorController::checkConfig(const QString &serverId)
{
    if (!isLoggedIn()) {
        return;
    }
    const QJsonObject entry = managedConfigs().value(serverId).toObject();
    const QString configId = entry.value(entryKey::configId).toString();
    if (configId.isEmpty()) {
        return;
    }

    m_api->configStatus(configId, entry.value(entryKey::revision).toString(), [this, serverId, configId](const CoordinatorApi::Reply &reply) {
        if (reply.networkError) {
            qWarning() << "CoordinatorController: status check failed for" << configId << reply.errorMessage;
            return;
        }
        if (reply.unauthorized() || reply.notMember()) {
            handleAuthError(reply);
            return;
        }
        if (!isManagedServer(serverId)) {
            return; // removed while the request was in flight
        }

        const QString name = managedConfigs().value(serverId).toObject().value(entryKey::name).toString();
        auto markRevoked = [this, serverId, name]() {
            if (setManagedState(serverId, QStringLiteral("revoked"))) {
                emit errorOccurred(tr("Конфиг «%1» отозван").arg(name));
            }
        };

        if (reply.notFound()) {
            markRevoked();
            return;
        }
        if (!reply.ok()) {
            qWarning() << "CoordinatorController: status check failed for" << configId << reply.httpStatus << reply.errorMessage;
            return;
        }
        m_notMemberReported = false;

        const QString network = reply.body.value("network").toString();
        if (!network.isEmpty() && network != m_lastNetworkState) {
            m_lastNetworkState = network;
            emit networkStateChanged(network);
        }

        const QString state = reply.body.value("state").toString();
        if (state == QLatin1String("ok")) {
            setManagedState(serverId, state);
        } else if (state == QLatin1String("outdated")) {
            m_api->getConfig(configId, [this, serverId, markRevoked](const CoordinatorApi::Reply &configReply) {
                if (!isManagedServer(serverId)) {
                    return;
                }
                if (configReply.ok()) {
                    if (applyConfig(configReply.body, serverId)) {
                        reconnectIfActive(serverId);
                    }
                } else if (configReply.unauthorized() || configReply.notMember()) {
                    handleAuthError(configReply);
                } else if (configReply.notFound()) {
                    markRevoked();
                } else {
                    qWarning() << "CoordinatorController: config download failed:" << configReply.httpStatus << configReply.errorMessage;
                }
            });
        } else if (state == QLatin1String("suspended")) {
            if (setManagedState(serverId, state)) {
                emit errorOccurred(tr("Доступ к VPN приостановлен: вы не состоите в чате"));
            }
        } else if (state == QLatin1String("revoked")) {
            markRevoked();
        } else {
            qWarning() << "CoordinatorController: unknown config state" << state;
        }
    });
}

bool CoordinatorController::applyConfig(const QJsonObject &config, const QString &existingServerId)
{
    const QString configId = config.value("id").toString();
    const QString name = config.value("name").toString();
    const QString awgText = config.value("awg").toString();

    const ImportController::ImportResult result = m_importController->extractConfigFromData(awgText);
    if (configId.isEmpty() || awgText.isEmpty() || result.errorCode != ErrorCode::NoError || result.config.isEmpty()) {
        qWarning() << "CoordinatorController: failed to parse config" << configId << errorString(result.errorCode);
        emit errorOccurred(tr("Не удалось импортировать конфиг «%1» с координатора").arg(name));
        return false;
    }

    // The server is named after the exit country ("serverName"); older coordinators only send the config name
    const QString displayName = config.value("serverName").toString().trimmed().isEmpty()
            ? name
            : config.value("serverName").toString().trimmed();

    QJsonObject serverJson = result.config;
    serverJson[configKey::description] = displayName;
    const serverConfigUtils::ConfigType kind = serverConfigUtils::configTypeFromJson(serverJson);

    const bool replace = !existingServerId.isEmpty() && m_serversRepository->indexOfServerId(existingServerId) >= 0;
    QString serverId = existingServerId;
    if (replace) {
        // Keep a name the user gave the server in the app; a name the coordinator set follows the coordinator.
        const auto current = m_serversRepository->nativeConfig(existingServerId);
        const QJsonObject previous = managedConfigs().value(existingServerId).toObject();
        const QString previousDisplayName = previous.contains(entryKey::displayName)
                ? previous.value(entryKey::displayName).toString()
                : previous.value(entryKey::name).toString();
        if (current.has_value() && !current->description.isEmpty() && current->description != previousDisplayName) {
            serverJson[configKey::description] = current->description;
        }
        m_serversRepository->editServer(existingServerId, serverJson, kind);
    } else {
        if (!existingServerId.isEmpty()) {
            setManagedEntry(existingServerId, QJsonObject()); // stale entry, server is gone
        }
        const bool wasConnected = m_connectionController->isConnected();
        serverId = m_serversRepository->addServer(QString(), serverJson, kind);
        if (serverId.isEmpty()) {
            emit errorOccurred(tr("Не удалось добавить конфиг «%1»").arg(name));
            return false;
        }
        if (!wasConnected || m_serversRepository->serversCount() == 1) {
            m_serversRepository->setDefaultServer(serverId);
        }
    }

    QJsonObject entry;
    entry[entryKey::configId] = configId;
    entry[entryKey::name] = name;
    entry[entryKey::displayName] = displayName;
    entry[entryKey::revision] = config.value("revision").toString();
    entry[entryKey::state] = QStringLiteral("ok");
    setManagedEntry(serverId, entry);

    if (replace) {
        emit configReplaced(serverId);
    } else {
        emit configAdded(serverId);
    }
    return true;
}

void CoordinatorController::handleAuthError(const CoordinatorApi::Reply &reply)
{
    if (reply.unauthorized()) {
        if (!isLoggedIn()) {
            return; // already handled by a parallel request
        }
        m_appSettingsRepository->clearCoordinatorAccount();
        m_api->setToken(QString());
        m_refreshTimer.stop();
        emit loggedInChanged();
        emit errorOccurred(tr("Сессия координатора недействительна, войдите снова по коду из бота"));
    } else if (reply.notMember()) {
        if (!m_notMemberReported) {
            m_notMemberReported = true;
            emit errorOccurred(tr("Вы больше не участник чата — доступ к VPN приостановлен"));
        }
    }
}

QString CoordinatorController::describeError(const CoordinatorApi::Reply &reply) const
{
    if (reply.networkError) {
        return tr("Координатор недоступен: %1").arg(reply.errorMessage);
    }
    if (reply.errorCode == QLatin1String("invalid_code")) {
        return tr("Неверный или просроченный код");
    }
    if (reply.errorCode == QLatin1String("name_taken")) {
        return tr("Имя уже занято");
    }
    if (reply.errorCode == QLatin1String("invalid_name")) {
        return tr("Имя может состоять только из латинских букв, цифр и дефисов, до 32 символов");
    }
    if (reply.notMember()) {
        return tr("Вы больше не участник чата — доступ к VPN приостановлен");
    }
    if (reply.unauthorized()) {
        return tr("Сессия координатора недействительна, войдите снова по коду из бота");
    }
    if (reply.notFound()) {
        return tr("Конфиг не найден на координаторе");
    }
    if (!reply.errorMessage.isEmpty()) {
        return reply.errorMessage;
    }
    return tr("Ошибка координатора (HTTP %1)").arg(reply.httpStatus);
}

void CoordinatorController::setManagedEntry(const QString &serverId, const QJsonObject &entry)
{
    QJsonObject managed = managedConfigs();
    if (entry.isEmpty()) {
        if (!managed.contains(serverId)) {
            return;
        }
        managed.remove(serverId);
    } else {
        if (managed.value(serverId).toObject() == entry) {
            return;
        }
        managed[serverId] = entry;
    }
    m_appSettingsRepository->setCoordinatorConfigs(managed);
    emit managedConfigsChanged();
}

bool CoordinatorController::setManagedState(const QString &serverId, const QString &state)
{
    QJsonObject entry = managedConfigs().value(serverId).toObject();
    if (entry.isEmpty() || entry.value(entryKey::state).toString() == state) {
        return false;
    }
    entry[entryKey::state] = state;
    setManagedEntry(serverId, entry);
    return true;
}

void CoordinatorController::reconnectIfActive(const QString &serverId)
{
    if (!m_connectionController->isConnected() || m_serversRepository->defaultServerId() != serverId) {
        return;
    }
    m_pendingReconnectServerId = serverId;
    m_connectionController->closeConnection();
}

void CoordinatorController::allowThroughKillSwitch()
{
#ifdef AMNEZIA_DESKTOP
    if (m_appSettingsRepository->isStrictKillSwitchEnabled()) {
        const QString host = m_api->host();
        QStringList ips;
        const QString ip = host.isEmpty() ? QString() : NetworkUtilities::getIPAddress(host);
        if (!ip.isEmpty()) {
            ips.append(ip);
        }
        if (!m_api->fallbackAddress().isEmpty() && !ips.contains(m_api->fallbackAddress())) {
            ips.append(m_api->fallbackAddress()); // DNS may be unreachable through a dead tunnel
        }
        if (!ips.isEmpty()) {
            IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
                QRemoteObjectPendingReply<bool> reply = iface->addKillSwitchAllowedRange(ips);
                if (!reply.waitForFinished(1000) || !reply.returnValue())
                    qWarning() << "CoordinatorController::allowThroughKillSwitch(): Failed to execute remote addKillSwitchAllowedRange call";
            });
        }
    }
#endif
}

QString CoordinatorController::deviceLabel() const
{
    const QString name = deviceName::platformName();
    const QString label = name.isEmpty() ? QSysInfo::prettyProductName()
                                         : QStringLiteral("%1 (%2)").arg(name, QSysInfo::prettyProductName());
    return label.left(64); // the coordinator rejects longer labels
}
