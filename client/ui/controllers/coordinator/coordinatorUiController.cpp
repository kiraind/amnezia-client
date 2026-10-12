#include "coordinatorUiController.h"

#include <QJsonArray>
#include <QRegularExpression>

#include "core/controllers/coordinator/coordinatorController.h"

CoordinatorUiController::CoordinatorUiController(CoordinatorController *coordinatorController, QObject *parent)
    : QObject(parent), m_coordinatorController(coordinatorController)
{
    connect(m_coordinatorController, &CoordinatorController::loggedInChanged, this, &CoordinatorUiController::loggedInChanged);
    connect(m_coordinatorController, &CoordinatorController::loginFinished, this, &CoordinatorUiController::loginFinished);
    connect(m_coordinatorController, &CoordinatorController::configAdded, this, &CoordinatorUiController::configAdded);
    connect(m_coordinatorController, &CoordinatorController::errorOccurred, this, &CoordinatorUiController::errorOccurred);

    connect(m_coordinatorController, &CoordinatorController::remoteConfigsUpdated, this, [this](const QJsonArray &configs) {
        m_remoteConfigs = configs.toVariantList();
        emit remoteConfigsChanged();
    });

    connect(m_coordinatorController, &CoordinatorController::busyChanged, this, [this](bool busy) {
        if (m_busy == busy) {
            return;
        }
        m_busy = busy;
        emit busyChanged();
    });

    connect(m_coordinatorController, &CoordinatorController::networkStateChanged, this, [this](const QString &network) {
        if (m_networkState == network) {
            return;
        }
        m_networkState = network;
        emit networkStateChanged();
    });

    // Remote configs reflect the "inApp"/state of local servers, drop them when the account changes
    connect(m_coordinatorController, &CoordinatorController::loggedInChanged, this, [this]() {
        if (!m_coordinatorController->isLoggedIn() && !m_remoteConfigs.isEmpty()) {
            m_remoteConfigs.clear();
            emit remoteConfigsChanged();
        }
    });
}

bool CoordinatorUiController::isLoggedIn() const
{
    return m_coordinatorController->isLoggedIn();
}

QString CoordinatorUiController::username() const
{
    return m_coordinatorController->username();
}

QString CoordinatorUiController::baseUrl() const
{
    return m_coordinatorController->baseUrl();
}

QString CoordinatorUiController::defaultBaseUrl() const
{
#ifdef COORDINATOR_API_URL
    return QStringLiteral(COORDINATOR_API_URL);
#else
    return QString();
#endif
}

bool CoordinatorUiController::isBusy() const
{
    return m_busy;
}

QVariantList CoordinatorUiController::remoteConfigs() const
{
    return m_remoteConfigs;
}

QString CoordinatorUiController::networkState() const
{
    return m_networkState;
}

void CoordinatorUiController::login(const QString &baseUrl, const QString &code)
{
    m_coordinatorController->login(baseUrl.trimmed(), code.trimmed());
}

void CoordinatorUiController::logout()
{
    m_coordinatorController->logout();
}

void CoordinatorUiController::fetchRemoteConfigs()
{
    m_coordinatorController->fetchRemoteConfigs();
}

void CoordinatorUiController::createConfig(const QString &name)
{
    m_coordinatorController->createConfig(name);
}

void CoordinatorUiController::addConfigToApp(const QString &configId)
{
    m_coordinatorController->addConfigToApp(configId);
}

void CoordinatorUiController::deleteConfig(const QString &configId)
{
    m_coordinatorController->deleteConfig(configId);
}

void CoordinatorUiController::refreshAll()
{
    m_coordinatorController->refreshAll();
}

bool CoordinatorUiController::isValidConfigName(const QString &name) const
{
    static const QRegularExpression configNameRegExp(QStringLiteral("^[a-z0-9-]{1,32}$"));
    return configNameRegExp.match(name).hasMatch();
}

bool CoordinatorUiController::isManagedServer(const QString &serverId) const
{
    return m_coordinatorController->isManagedServer(serverId);
}
