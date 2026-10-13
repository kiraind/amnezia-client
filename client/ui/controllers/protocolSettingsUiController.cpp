#include "protocolSettingsUiController.h"

#include "core/models/containerConfig.h"
#include "core/models/protocols/awgProtocolConfig.h"
#include "core/models/protocols/wireGuardProtocolConfig.h"
#include "core/utils/protocolEnum.h"

ProtocolSettingsUiController::ProtocolSettingsUiController(ServersController *serversController,
                                                           ProtocolsModel *protocolsModel, AwgConfigModel *awgConfigModel,
                                                           WireGuardConfigModel *wireGuardConfigModel, QObject *parent)
    : QObject(parent),
      m_serversController(serversController),
      m_protocolsModel(protocolsModel),
      m_awgConfigModel(awgConfigModel),
      m_wireGuardConfigModel(wireGuardConfigModel)
{
}

void ProtocolSettingsUiController::updateProtocols(const QString &serverId, int containerIndex)
{
    const DockerContainer container = static_cast<DockerContainer>(containerIndex);
    ContainerConfig containerConfig = m_serversController->getContainerConfig(serverId, container);
    containerConfig.container = container;
    m_protocolsModel->updateModel(containerConfig);
}

void ProtocolSettingsUiController::openClientSettings(const QString &serverId, int containerIndex, int protocolIndex)
{
    updateProtocolConfigModel(serverId, containerIndex, protocolIndex);
}

void ProtocolSettingsUiController::updateClientConfig(const QString &serverId, int containerIndex, int protocolIndex,
                                                      bool closePage)
{
    const DockerContainer container = static_cast<DockerContainer>(containerIndex);

    ContainerConfig containerConfig;
    containerConfig.container = container;
    switch (static_cast<Proto>(protocolIndex)) {
    case Proto::Awg: containerConfig.protocolConfig = m_awgConfigModel->getProtocolConfig(); break;
    case Proto::WireGuard: containerConfig.protocolConfig = m_wireGuardConfigModel->getProtocolConfig(); break;
    default: return;
    }

    const ErrorCode errorCode = m_serversController->updateClientConfig(serverId, container, containerConfig);
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return;
    }

    updateProtocols(serverId, containerIndex);
    updateProtocolConfigModel(serverId, containerIndex, protocolIndex);
    emit updateContainerFinished(tr("Settings updated successfully"), closePage);
}

void ProtocolSettingsUiController::updateProtocolConfigModel(const QString &serverId, int containerIndex, int protocolIndex)
{
    const DockerContainer container = static_cast<DockerContainer>(containerIndex);
    const ContainerConfig containerConfig = m_serversController->getContainerConfig(serverId, container);

    switch (static_cast<Proto>(protocolIndex)) {
    case Proto::Awg:
        if (const auto *config = containerConfig.getAwgProtocolConfig()) {
            m_awgConfigModel->updateModel(container, *config);
        }
        break;
    case Proto::WireGuard:
        if (const auto *config = containerConfig.getWireGuardProtocolConfig()) {
            m_wireGuardConfigModel->updateModel(container, *config);
        }
        break;
    default: break;
    }
}
