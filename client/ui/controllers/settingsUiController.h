#ifndef SETTINGSUICONTROLLER_H
#define SETTINGSUICONTROLLER_H

#include <QObject>
#include <QRegularExpression>

#include "core/controllers/settingsController.h"
#include "core/controllers/serversController.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"

class SettingsUiController : public QObject
{
    Q_OBJECT
public:
    explicit SettingsUiController(SettingsController* settingsController,
                                 ServersController* serversController,
                                 QObject *parent = nullptr);

    Q_PROPERTY(QString primaryDns READ getPrimaryDns WRITE setPrimaryDns NOTIFY primaryDnsChanged)
    Q_PROPERTY(QString secondaryDns READ getSecondaryDns WRITE setSecondaryDns NOTIFY secondaryDnsChanged)
    Q_PROPERTY(bool isLoggingEnabled READ isLoggingEnabled WRITE toggleLogging NOTIFY loggingStateChanged)
    Q_PROPERTY(bool isNotificationPermissionGranted READ isNotificationPermissionGranted NOTIFY onNotificationStateChanged)
    Q_PROPERTY(bool isKillSwitchEnabled READ isKillSwitchEnabled WRITE toggleKillSwitch NOTIFY killSwitchEnabledChanged)
    Q_PROPERTY(bool strictKillSwitchEnabled READ isStrictKillSwitchEnabled WRITE toggleStrictKillSwitch NOTIFY strictKillSwitchEnabledChanged)

    Q_PROPERTY(bool autoStartEnabled READ isAutoStartEnabled NOTIFY autoStartChanged)
    Q_PROPERTY(bool startMinimized READ isStartMinimizedEnabled NOTIFY startMinimizedChanged)

public slots:
    void toggleAmneziaDns(bool enable);
    bool isAmneziaDnsEnabled();

    QString getPrimaryDns();
    void setPrimaryDns(const QString &dns);

    QString getSecondaryDns();
    void setSecondaryDns(const QString &dns);

    // Validator for the DNS fields (moved from the removed InstallController)
    QRegularExpression ipAddressRegExp();

    bool isLoggingEnabled();
    void toggleLogging(bool enable);

    void openLogsFolder();
    void openServiceLogsFolder();
    void exportLogsFile(const QString &fileName);
    void exportServiceLogsFile(const QString &fileName);
    void clearLogs();

    QString getAppVersion();

    void clearSettings();

    bool isAutoConnectEnabled();
    void toggleAutoConnect(bool enable);

    bool isAutoStartEnabled();
    void toggleAutoStart(bool enable);

    bool isStartMinimizedEnabled();
    void toggleStartMinimized(bool enable);

    bool isScreenshotsEnabled();
    void toggleScreenshotsEnabled(bool enable);

    bool isCameraPresent();

    bool isKillSwitchEnabled();
    void toggleKillSwitch(bool enable);

    bool isStrictKillSwitchEnabled();
    void toggleStrictKillSwitch(bool enable);

    bool isNotificationPermissionGranted();
    void requestNotificationPermission();

    QString getInstallationUuid();

    bool isOnTv();

signals:
    void primaryDnsChanged();
    void secondaryDnsChanged();
    void loggingStateChanged();
    void killSwitchEnabledChanged();
    void strictKillSwitchEnabledChanged(bool enabled);

    void changeSettingsFinished(const QString &finishedMessage);
    void errorOccurred(ErrorCode errorCode);

    void saveFile(const QString &fileName, const QString &data);

    void amneziaDnsToggled(bool enable);

    void loggingDisableByWatcher();

    void resetLanguageToSystem();

    void onNotificationStateChanged();

    void activityPaused();
    void activityResumed();

    void autoStartChanged();
    void startMinimizedChanged();

private:
    SettingsController* m_settingsController;
    ServersController* m_serversController;
};

#endif
