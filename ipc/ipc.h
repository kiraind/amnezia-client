#ifndef IPC_H
#define IPC_H

#include <QObject>
#include <QString>
#include <QRegularExpression>
#include <QSet>

#include "../client/core/utils/utilities.h"
#include "version.h"

// Derived from the branded app name so a side-by-side install of another build gets its own socket/pipe
#define IPC_SERVICE_URL "local:" APPLICATION_NAME "IpcInterface"

namespace amnezia {

inline QString getIpcServiceUrl() {
#ifdef Q_OS_WIN
    return IPC_SERVICE_URL;
#else
    return QString("/tmp/%1").arg(IPC_SERVICE_URL);
#endif
}

} // namespace amnezia

#endif // IPC_H
