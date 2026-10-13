/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "daemonlocalserver.h"

#include <QDir>
#include <QFileInfo>
#include <QLocalSocket>

#include "daemonlocalserverconnection.h"
#include "leakdetector.h"
#include "logger.h"
#include "version.h"

#if defined(MZ_MACOS) || defined(MZ_LINUX)
#  include <sys/stat.h>
#  include <sys/types.h>
#  include <unistd.h>

#endif

namespace {
// Derived from the branded app name so side-by-side installs don't share a daemon socket
QString daemonDirName() { return QString(APPLICATION_NAME).toLower(); }
}  // namespace

namespace {
Logger logger("DaemonLocalServer");
}  // namespace

DaemonLocalServer::DaemonLocalServer(QObject* parent) : QObject(parent) {
  MZ_COUNT_CTOR(DaemonLocalServer);
}

DaemonLocalServer::~DaemonLocalServer() { MZ_COUNT_DTOR(DaemonLocalServer); }

bool DaemonLocalServer::initialize() {
  m_server.setSocketOptions(QLocalServer::WorldAccessOption);

  QString path = daemonPath();
  logger.debug() << "Server path:" << path;

  if (QFileInfo::exists(path)) {
    QFile::remove(path);
  }

  if (!m_server.listen(path)) {
    logger.error() << "Failed to listen the daemon path";
    return false;
  }

  connect(&m_server, &QLocalServer::newConnection, [&] {
    logger.debug() << "New connection received";

    if (!m_server.hasPendingConnections()) {
      return;
    }

    QLocalSocket* socket = m_server.nextPendingConnection();
    Q_ASSERT(socket);

    DaemonLocalServerConnection* connection =
        new DaemonLocalServerConnection(&m_server, socket);
    connect(socket, &QLocalSocket::disconnected, connection,
            &DaemonLocalServerConnection::deleteLater);
  });

  return true;
}

QString DaemonLocalServer::daemonPath() const {
#if defined(MZ_WINDOWS)
  return "\\\\.\\pipe\\" + daemonDirName();
#endif
#if defined(MZ_MACOS) || defined(MZ_LINUX)
  const QString dirName = daemonDirName();
  const QString TMP_PATH = "/tmp/" + dirName + ".socket";
  const QString VAR_DIR = "/var/run/" + dirName;
  const QString VAR_PATH = VAR_DIR + "/daemon.socket";

  QDir dir("/var/run");
  if (!dir.exists()) {
    logger.warning() << "/var/run doesn't exist. Fallback /tmp.";
    return TMP_PATH;
  }

  if (dir.exists(dirName)) {
    logger.debug() << VAR_DIR << "seems to be usable";
    return VAR_PATH;
  }

  if (!dir.mkdir(dirName)) {
    logger.warning() << "Failed to create" << VAR_DIR;
    return TMP_PATH;
  }

  if (chmod(qPrintable(VAR_DIR), S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
    logger.warning()
        << "Failed to set the right permissions to" << VAR_DIR;
    return TMP_PATH;
  }

  return VAR_PATH;
#endif
}
