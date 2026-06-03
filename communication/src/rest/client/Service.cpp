#include "rest/client/Service.h"

#include <QJsonDocument>
#include <QLoggingCategory>
#include <QNetworkRequest>

Q_LOGGING_CATEGORY(RestClientService, "RestClientService")

namespace Common::Communication::Rest::Client
{

constexpr int NETWORK_TIMEOUT_MS = 30 * 1000;
const QString PROPERTY_SERVER_URL_DEFAULT = QStringLiteral("http://127.0.0.1:5000");

Service::Service(QObject* parent)
    : QObject(parent),
      m_networkManager(this),
      m_serverUrl(PROPERTY_SERVER_URL_DEFAULT)
{
}

QString Service::serverUrl() const
{
    return m_serverUrl;
}

void Service::setServerUrl(const QString& url)
{
    if (m_serverUrl == url) {
        return;
    }

    m_serverUrl = url;
    emit serverUrlChanged();
}

void Service::get(const QString& endpoint, ResponseCallback callback)
{
    startJsonRequest(m_networkManager.get(createRequest(endpoint)), std::move(callback));
}

void Service::post(const QString& endpoint, const QJsonObject& payload, ResponseCallback callback)
{
    QNetworkRequest request = createRequest(endpoint);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    startJsonRequest(m_networkManager.post(request, QJsonDocument(payload).toJson()), std::move(callback));
}

void Service::put(const QString& endpoint, const QJsonObject& payload, ResponseCallback callback)
{
    QNetworkRequest request = createRequest(endpoint);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    startJsonRequest(m_networkManager.put(request, QJsonDocument(payload).toJson()), std::move(callback));
}

void Service::deleteResource(const QString& endpoint, ResponseCallback callback)
{
    startJsonRequest(m_networkManager.deleteResource(createRequest(endpoint)), std::move(callback));
}

void Service::download(const QString& endpoint, DataCallback callback)
{
    startBinaryRequest(m_networkManager.get(createRequest(endpoint)), std::move(callback));
}

QNetworkRequest Service::createRequest(const QString& endpoint) const
{
    QString urlStr = m_serverUrl;
    if (!urlStr.endsWith('/') && !endpoint.startsWith('/')) {
        urlStr += '/';
    }
    urlStr += endpoint;

    QNetworkRequest request{QUrl(urlStr)};
    request.setTransferTimeout(NETWORK_TIMEOUT_MS);
    return request;
}

void Service::startJsonRequest(QNetworkReply* reply, ResponseCallback callback)
{
    m_pendingRequests.insert(reply, PendingRequest{reply, std::move(callback), DataCallback(), false});
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleResponse(reply);
    });
}

void Service::startBinaryRequest(QNetworkReply* reply, DataCallback callback)
{
    m_pendingRequests.insert(reply, PendingRequest{reply, ResponseCallback(), std::move(callback), true});
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleResponse(reply);
    });
}

void Service::handleResponse(QNetworkReply* reply)
{
    reply->deleteLater();
    if (!m_pendingRequests.contains(reply)) {
        return;
    }

    const PendingRequest pending = m_pendingRequests.take(reply);
    if (reply->error() != QNetworkReply::NoError) {
        handleError(reply, reply->errorString());
        if (pending.isBinaryDownload && pending.dataCallback) {
            pending.dataCallback(false, QByteArray(), reply->errorString());
        }
        else if (pending.responseCallback) {
            pending.responseCallback(false, QJsonObject(), reply->errorString());
        }
        return;
    }

    const QByteArray data = reply->readAll();
    if (pending.isBinaryDownload) {
        if (pending.dataCallback) {
            pending.dataCallback(true, data, QString());
        }
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isObject()) {
        if (pending.responseCallback) {
            pending.responseCallback(true, doc.object(), QString());
        }
        return;
    }

    if (pending.responseCallback) {
        pending.responseCallback(false, QJsonObject(), QStringLiteral("Invalid JSON response"));
    }
}

void Service::handleError(QNetworkReply* reply, const QString& errorString) const
{
    qCWarning(RestClientService) << "REST Error:" << errorString << "URL:" << reply->url().toString();
}

} // namespace Common::Communication::Rest::Client