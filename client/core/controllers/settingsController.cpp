#include "settingsController.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QOperatingSystemVersion>

#include "version.h"
#include "ui/utils/qAutoStart.h"
#include "logger.h"
#ifdef Q_OS_ANDROID
    #include "platforms/android/android_controller.h"
#endif

SettingsController::SettingsController(SecureServersRepository* serversRepository,
                                     SecureAppSettingsRepository* appSettingsRepository,
                                     QObject* parent)
    : QObject(parent),
      m_serversRepository(serversRepository),
      m_appSettingsRepository(appSettingsRepository)
{
    m_appVersion = QString("%1 (%2, %3)").arg(QString(APP_FULL_VERSION), __DATE__, GIT_COMMIT_HASH);
}

void SettingsController::toggleAmneziaDns(bool enable)
{
    m_appSettingsRepository->setUseAmneziaDns(enable);
}

bool SettingsController::isAmneziaDnsEnabled() const
{
    return m_appSettingsRepository->useAmneziaDns();
}

QString SettingsController::getPrimaryDns() const
{
    return m_appSettingsRepository->primaryDns();
}

void SettingsController::setPrimaryDns(const QString &dns)
{
    m_appSettingsRepository->setPrimaryDns(dns);
}

QString SettingsController::getSecondaryDns() const
{
    return m_appSettingsRepository->secondaryDns();
}

void SettingsController::setSecondaryDns(const QString &dns)
{
    m_appSettingsRepository->setSecondaryDns(dns);
}

bool SettingsController::isLoggingEnabled() const
{
    return m_appSettingsRepository->isSaveLogs();
}

void SettingsController::toggleLogging(bool enable)
{
    m_appSettingsRepository->setSaveLogs(enable);
#ifndef Q_OS_ANDROID
    if (!enable) {
        Logger::deInit();
    } else {
        if (!Logger::init(false)) {
            qWarning() << "Initialization of debug subsystem failed";
        }
    }
#endif
    Logger::setServiceLogsEnabled(enable);

    if (enable) {
        m_appSettingsRepository->setLogEnableDate(QDateTime::currentDateTime());
    }
}

void SettingsController::clearLogs()
{
#ifdef Q_OS_ANDROID
    AndroidController::instance()->clearLogs();
#else
    Logger::clearLogs(false);
    Logger::clearServiceLogs();
#endif
}

QString SettingsController::getAppVersion() const
{
    return m_appVersion;
}

void SettingsController::clearSettings()
{
    m_appSettingsRepository->clearSettings();

    m_serversRepository->clearServers();

    emit siteSplitTunnelingRouteModeChanged(RouteMode::VpnOnlyForwardSites);
    emit siteSplitTunnelingToggled(false);

    emit appSplitTunnelingRouteModeChanged(AppsRouteMode::VpnAllExceptApps);
    emit appSplitTunnelingToggled(false);

    toggleAutoStart(false);
}

bool SettingsController::isAutoConnectEnabled() const
{
    return m_appSettingsRepository->isAutoConnect();
}

void SettingsController::toggleAutoConnect(bool enable)
{
    m_appSettingsRepository->setAutoConnect(enable);
}

bool SettingsController::isAutoStartEnabled() const
{
    return Autostart::isAutostart();
}

void SettingsController::toggleAutoStart(bool enable)
{
    Autostart::setAutostart(enable);
    if (!enable) {
        toggleStartMinimized(false);
    }
}

bool SettingsController::isStartMinimizedEnabled() const
{
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    if (!isAutoStartEnabled()) {
        return false;
    }
#endif
    return m_appSettingsRepository->isStartMinimized();
}

void SettingsController::toggleStartMinimized(bool enable)
{
    m_appSettingsRepository->setStartMinimized(enable);
}

bool SettingsController::isScreenshotsEnabled() const
{
    return m_appSettingsRepository->isScreenshotsEnabled();
}

void SettingsController::toggleScreenshotsEnabled(bool enable)
{
    m_appSettingsRepository->setScreenshotsEnabled(enable);
}

bool SettingsController::isKillSwitchEnabled() const
{
    return m_appSettingsRepository->isKillSwitchEnabled();
}

void SettingsController::toggleKillSwitch(bool enable)
{
    m_appSettingsRepository->setKillSwitchEnabled(enable);
}

bool SettingsController::isStrictKillSwitchEnabled() const
{
    return m_appSettingsRepository->isStrictKillSwitchEnabled();
}

void SettingsController::toggleStrictKillSwitch(bool enable)
{
    m_appSettingsRepository->setStrictKillSwitchEnabled(enable);
}

QString SettingsController::getInstallationUuid(bool createIfNotExists) const
{
    return m_appSettingsRepository->getInstallationUuid(createIfNotExists);
}

void SettingsController::checkIfNeedDisableLogs()
{
    if (m_appSettingsRepository->isSaveLogs()) {
        m_loggingDisableDate = m_appSettingsRepository->getLogEnableDate().addDays(14);
        if (m_loggingDisableDate <= QDateTime::currentDateTime()) {
            toggleLogging(false);
            clearLogs();
        }
    }
}

QLocale SettingsController::getAppLanguage() const
{
    return m_appSettingsRepository->getAppLanguage();
}

void SettingsController::setAppLanguage(const QLocale &locale)
{
    m_appSettingsRepository->setAppLanguage(locale);
}

QString SettingsController::nextAvailableServerName() const
{
    return m_serversRepository->nextAvailableServerName();
}

