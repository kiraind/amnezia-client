#include "coordinatorApi.h"

#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslError>
#include <QStringList>
#include <QUrl>
#include <QUrlQuery>
#include <QUuid>

#include <memory>

#include "core/utils/constants/apiConstants.h"
#include "version.h"

namespace
{
    QString encodeSegment(const QString &segment)
    {
        return QString::fromLatin1(QUrl::toPercentEncoding(segment));
    }
}

CoordinatorApi::CoordinatorApi(QNetworkAccessManager *networkManager, QObject *parent)
    : QObject(parent), m_networkManager(networkManager)
{
}

void CoordinatorApi::setBaseUrl(const QString &baseUrl)
{
    QString url = baseUrl.trimmed();
    while (url.endsWith(QLatin1Char('/'))) {
        url.chop(1);
    }
    m_baseUrl = url;
}

QString CoordinatorApi::baseUrl() const
{
    return m_baseUrl;
}

void CoordinatorApi::setToken(const QString &token)
{
    m_token = token;
}

void CoordinatorApi::login(const QString &code, const QString &deviceLabel, const Callback &callback)
{
    QJsonObject body;
    body[QStringLiteral("code")] = code;
    if (!deviceLabel.isEmpty()) {
        body[QStringLiteral("deviceLabel")] = deviceLabel;
    }
    send("POST", QStringLiteral("/auth/login"), &body, false, callback);
}

void CoordinatorApi::logout(const Callback &callback)
{
    const QJsonObject body;
    send("POST", QStringLiteral("/auth/logout"), &body, true, callback);
}

void CoordinatorApi::me(const Callback &callback)
{
    send("GET", QStringLiteral("/me"), nullptr, true, callback);
}

void CoordinatorApi::createConfig(const QString &name, const Callback &callback)
{
    QJsonObject body;
    body[QStringLiteral("name")] = name;
    send("POST", QStringLiteral("/configs"), &body, true, callback);
}

void CoordinatorApi::getConfig(const QString &configId, const Callback &callback)
{
    send("GET", QStringLiteral("/configs/") + encodeSegment(configId), nullptr, true, callback);
}

void CoordinatorApi::configStatus(const QString &configId, const QString &revision, const Callback &callback)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("revision"), revision);
    send("GET", QStringLiteral("/configs/%1/status?%2").arg(encodeSegment(configId), query.toString(QUrl::FullyEncoded)), nullptr,
         true, callback);
}

void CoordinatorApi::deleteConfig(const QString &configId, const Callback &callback)
{
    send("DELETE", QStringLiteral("/configs/") + encodeSegment(configId), nullptr, true, callback);
}

QString CoordinatorApi::host() const
{
    return QUrl(m_baseUrl).host();
}

void CoordinatorApi::send(const QByteArray &verb, const QString &path, const QJsonObject *body, bool authorized,
                          const Callback &callback)
{
    QNetworkRequest request(QUrl(m_baseUrl + QStringLiteral("/v1") + path, QUrl::StrictMode));
    request.setTransferTimeout(apiDefs::requestTimeoutMsecs);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("X-Client-Request-ID", QUuid::createUuid().toString(QUuid::WithoutBraces).toUtf8());
    request.setRawHeader("X-App-Version", QByteArray(APP_FULL_VERSION));
    if (authorized && !m_token.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_token.toUtf8());
    }

    const QByteArray data = body ? QJsonDocument(*body).toJson(QJsonDocument::Compact) : QByteArray();

    QNetworkReply *reply = nullptr;
    if (verb == "GET") {
        reply = m_networkManager->get(request);
    } else if (verb == "POST") {
        reply = m_networkManager->post(request, data);
    } else if (verb == "DELETE") {
        reply = m_networkManager->deleteResource(request);
    } else {
        reply = m_networkManager->sendCustomRequest(request, verb, data);
    }

    auto sslErrors = std::make_shared<QStringList>();
    connect(reply, &QNetworkReply::sslErrors, this, [sslErrors](const QList<QSslError> &errors) {
        for (const QSslError &error : errors) {
            sslErrors->append(error.errorString());
        }
    });

    connect(reply, &QNetworkReply::finished, this, [reply, sslErrors, callback]() {
        Reply result;
        result.httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        result.networkError = result.httpStatus == 0 || !sslErrors->isEmpty()
                || reply->error() == QNetworkReply::OperationCanceledError || reply->error() == QNetworkReply::TimeoutError;

        const QByteArray responseBody = reply->readAll();
        if (!responseBody.isEmpty()) {
            const QJsonDocument document = QJsonDocument::fromJson(responseBody);
            if (document.isObject()) {
                result.body = document.object();
            }
        }

        const QJsonObject error = result.body.value(QStringLiteral("error")).toObject();
        result.errorCode = error.value(QStringLiteral("code")).toString();
        result.errorMessage = error.value(QStringLiteral("message")).toString();

        if (!sslErrors->isEmpty()) {
            result.errorMessage = QStringLiteral("SSL error: ") + sslErrors->join(QStringLiteral("; "));
        } else if (result.errorMessage.isEmpty() && !result.ok()) {
            result.errorMessage = reply->errorString();
        }

        reply->deleteLater();
        if (callback) {
            callback(result);
        }
    });
}
