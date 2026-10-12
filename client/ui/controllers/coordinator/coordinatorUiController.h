#ifndef COORDINATORUICONTROLLER_H
#define COORDINATORUICONTROLLER_H

#include <QObject>
#include <QVariantList>

class CoordinatorController;

// QML facade for CoordinatorController, exposed as the "CoordinatorController" context property.
class CoordinatorUiController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool loggedIn READ isLoggedIn NOTIFY loggedInChanged)
    Q_PROPERTY(QString username READ username NOTIFY loggedInChanged)
    Q_PROPERTY(QString baseUrl READ baseUrl NOTIFY loggedInChanged)
    Q_PROPERTY(QString defaultBaseUrl READ defaultBaseUrl CONSTANT)
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)
    // [{ id, name, revision, createdAt, inApp, state }] from the last fetchRemoteConfigs()
    Q_PROPERTY(QVariantList remoteConfigs READ remoteConfigs NOTIFY remoteConfigsChanged)
    Q_PROPERTY(QString networkState READ networkState NOTIFY networkStateChanged)

public:
    explicit CoordinatorUiController(CoordinatorController *coordinatorController, QObject *parent = nullptr);

    bool isLoggedIn() const;
    QString username() const;
    QString baseUrl() const;
    QString defaultBaseUrl() const; // COORDINATOR_API_URL compile definition, may be empty
    bool isBusy() const;
    QVariantList remoteConfigs() const;
    QString networkState() const;

    Q_INVOKABLE void login(const QString &baseUrl, const QString &code);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void fetchRemoteConfigs();
    Q_INVOKABLE void createConfig(const QString &name);
    Q_INVOKABLE void addConfigToApp(const QString &configId);
    Q_INVOKABLE void deleteConfig(const QString &configId);
    Q_INVOKABLE void refreshAll();
    Q_INVOKABLE bool isValidConfigName(const QString &name) const; // [a-z0-9-]{1,32}
    Q_INVOKABLE bool isManagedServer(const QString &serverId) const;

signals:
    void loggedInChanged();
    void busyChanged();
    void remoteConfigsChanged();
    void networkStateChanged();
    void loginFinished(bool success, const QString &errorText);
    void configAdded(const QString &serverId);
    void errorOccurred(const QString &text); // also routed to PageController.showErrorMessage by CoreSignalHandlers

private:
    CoordinatorController *m_coordinatorController;
    bool m_busy = false;
    QVariantList m_remoteConfigs;
    QString m_networkState;
};

#endif // COORDINATORUICONTROLLER_H
