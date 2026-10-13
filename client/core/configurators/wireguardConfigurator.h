#ifndef WIREGUARD_CONFIGURATOR_H
#define WIREGUARD_CONFIGURATOR_H

#include <QObject>

#include "configuratorBase.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"

// Used for both WireGuard and AmneziaWG: the stored client config only needs the DNS placeholders filled in.
class WireguardConfigurator : public ConfiguratorBase
{
    Q_OBJECT
public:
    explicit WireguardConfigurator(QObject *parent = nullptr);

    struct ConnectionData
    {
        QString clientPrivKey; // client private key
        QString clientPubKey;  // client public key
        QString clientIP;      // internal client IP address
        QString serverPubKey;  // tls-auth key
        QString pskKey;        // preshared key
        QString host;          // host ip
        QString port;
    };

    static ConnectionData genClientKeys();
};

#endif // WIREGUARD_CONFIGURATOR_H
