#ifndef COMMON_COMMUNICATION_REST_CLIENT_SERVICE_H
#define COMMON_COMMUNICATION_REST_CLIENT_SERVICE_H

#include <QJsonObject>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>

#include <functional>

namespace Common::Communication::Rest::Client
{

class Service : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString serverUrl READ serverUrl WRITE setServerUrl NOTIFY serverUrlChanged)

  public:
    using ResponseCallback = std::function<void(bool success, const QJsonObject& response, const QString& error)>;
    using DataCallback = std::function<void(bool success, const QByteArray& data, const QString& error)>;

    explicit Service(QObject* parent = nullptr);

    QString serverUrl() const;
    void setServerUrl(const QString& url);

    void get(const QString& endpoint, ResponseCallback callback);
    void post(const QString& endpoint, const QJsonObject& payload, ResponseCallback callback);
    void put(const QString& endpoint, const QJsonObject& payload, ResponseCallback callback);
    void deleteResource(const QString& endpoint, ResponseCallback callback);
    void download(const QString& endpoint, DataCallback callback);

  signals:
    void serverUrlChanged();

  private:
    struct PendingRequest
    {
        QNetworkReply* reply;
        ResponseCallback responseCallback;
        DataCallback dataCallback;
        bool isBinaryDownload;
    };

    QNetworkRequest createRequest(const QString& endpoint) const;
    void startJsonRequest(QNetworkReply* reply, ResponseCallback callback);
    void startBinaryRequest(QNetworkReply* reply, DataCallback callback);
    void handleResponse(QNetworkReply* reply);
    void handleError(QNetworkReply* reply, const QString& errorString) const;

    QNetworkAccessManager m_networkManager;
    QMap<QNetworkReply*, PendingRequest> m_pendingRequests;
    QString m_serverUrl;
};

} // namespace Common::Communication::Rest::Client

#endif // COMMON_COMMUNICATION_REST_CLIENT_SERVICE_H