#ifndef IKEV2_CONFIGURATOR_H
#define IKEV2_CONFIGURATOR_H

#include <QObject>

#include "configuratorBase.h"

// Fork: the server-side certificate generation over SSH was removed with self-hosted server management.
class Ikev2Configurator : public ConfiguratorBase
{
    Q_OBJECT
public:
    explicit Ikev2Configurator(QObject *parent = nullptr);
};

#endif // IKEV2_CONFIGURATOR_H
