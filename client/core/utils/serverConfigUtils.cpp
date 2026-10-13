#include "serverConfigUtils.h"

#include <QJsonArray>
#include <QJsonValue>

#include "core/models/selfhosted/selfHostedAdminServerConfig.h"
#include "core/utils/constants/configKeys.h"

namespace
{

bool hasThirdPartyConfig(const QJsonObject &json)
{
    const QJsonArray containersArray = json.value(amnezia::configKey::containers).toArray();
    for (const QJsonValue &val : containersArray) {
        const QJsonObject containerObj = val.toObject();
        for (auto it = containerObj.begin(); it != containerObj.end(); ++it) {
            if (it.key() == amnezia::configKey::container) {
                continue;
            }
            const QJsonObject protocolObj = it.value().toObject();
            if (protocolObj.contains(amnezia::configKey::isThirdPartyConfig)
                && protocolObj.value(amnezia::configKey::isThirdPartyConfig).toBool()) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

namespace serverConfigUtils
{

int configFormatVersion(const QJsonObject &serverConfigObject)
{
    return serverConfigObject.value(amnezia::configKey::formatVersion).toInt(0);
}

bool isConfigFormatVersionSupported(const QJsonObject &serverConfigObject)
{
    return configFormatVersion(serverConfigObject) <= currentConfigFormatVersion;
}

bool isServerFromApi(const QJsonObject &serverConfigObject)
{
    const int configVersion = serverConfigObject.value(amnezia::configKey::configVersion).toInt();
    switch (configVersion) {
    case ConfigSource::Telegram:
    case ConfigSource::AmneziaGateway:
        return true;
    default:
        return false;
    }
}

ConfigType configTypeFromJson(const QJsonObject &serverConfigObject)
{
    // Fork: servers issued by Amnezia's gateway (premium/free subscriptions) are not supported
    if (isServerFromApi(serverConfigObject)) {
        return ConfigType::Invalid;
    }

    if (hasThirdPartyConfig(serverConfigObject)) {
        return ConfigType::Native;
    }

    const amnezia::SelfHostedAdminServerConfig adminProbe =
            amnezia::SelfHostedAdminServerConfig::fromJson(serverConfigObject);
    return adminProbe.hasCredentials() ? ConfigType::SelfHostedAdmin : ConfigType::SelfHostedUser;
}

} // namespace serverConfigUtils
