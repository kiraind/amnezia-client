#ifndef IPCSERVER_H
#define IPCSERVER_H

#include <QLocalServer>
#include <QObject>
#include <QRemoteObjectNode>
#include <QJsonObject>
#include "../client/daemon/interfaceconfig.h"
#include "../client/mozilla/pinghelper.h"

#include "ipc.h"

#include "rep_ipc_interface_source.h"

class IpcServer : public IpcInterfaceSource
{
public:
    explicit IpcServer(QObject *parent = nullptr);

    virtual int routeAddList(const QString &gw, const QStringList &ips) override;
    virtual bool clearSavedRoutes() override;
    virtual bool routeDeleteList(const QString &gw, const QStringList &ips) override;
    virtual bool flushDns() override;
    virtual void resetIpStack() override;
    virtual void cleanUp() override;
    virtual void clearLogs() override;
    virtual void setLogsEnabled(bool enabled) override;
    virtual bool disableAllTraffic() override;
    virtual bool addKillSwitchAllowedRange(QStringList ranges) override;
    virtual bool resetKillSwitchAllowedRange(QStringList ranges) override;
    virtual bool disableKillSwitch() override;
    virtual bool refreshKillSwitch( bool enabled ) override;
    virtual bool startNetworkCheck(const QString& serverIpv4Gateway, const QString& deviceIpv4Address) override;
    virtual bool stopNetworkCheck() override;

private:
    PingHelper m_pingHelper;
};

#endif // IPCSERVER_H
