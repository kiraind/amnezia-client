#ifndef PROTOCOLSETTINGSUICONTROLLER_H
#define PROTOCOLSETTINGSUICONTROLLER_H

#include <QObject>

#include "core/controllers/serversController.h"
#include "core/utils/errorCodes.h"
#include "ui/models/protocolsModel.h"
#include "ui/models/protocols/awgConfigModel.h"
#include "ui/models/protocols/wireguardConfigModel.h"

// Fork: the client-side part of the removed InstallUiController. Opens and saves the per-device AWG/WireGuard
// connection settings of a stored server and fills ProtocolsModel for the protocol pages; never talks to a server.
class ProtocolSettingsUiController : public QObject
{
    Q_OBJECT
public:
    explicit ProtocolSettingsUiController(ServersController *serversController, ProtocolsModel *protocolsModel,
                                          AwgConfigModel *awgConfigModel, WireGuardConfigModel *wireGuardConfigModel,
                                          QObject *parent = nullptr);

public slots:
    void updateProtocols(const QString &serverId, int containerIndex);
    void openClientSettings(const QString &serverId, int containerIndex, int protocolIndex);
    void updateClientConfig(const QString &serverId, int containerIndex, int protocolIndex, bool closePage = true);

signals:
    void updateContainerFinished(const QString &message, bool closePage);
    void errorOccurred(ErrorCode errorCode);

private:
    void updateProtocolConfigModel(const QString &serverId, int containerIndex, int protocolIndex);

    ServersController *m_serversController;
    ProtocolsModel *m_protocolsModel;
    AwgConfigModel *m_awgConfigModel;
    WireGuardConfigModel *m_wireGuardConfigModel;
};

#endif // PROTOCOLSETTINGSUICONTROLLER_H
