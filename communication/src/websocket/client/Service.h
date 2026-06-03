#ifndef COMMON_COMMUNICATION_WEBSOCKET_CLIENT_SERVICE_H
#define COMMON_COMMUNICATION_WEBSOCKET_CLIENT_SERVICE_H

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QUrl>
#include <QWebSocket>

#include <functional>

#include "websocket/Types.h"

namespace Common::Communication::WebSocket::Client
{

class Service : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString serverUrl READ serverUrl WRITE setServerUrl NOTIFY serverUrlChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)

  public:
    using ResponseCallback = std::function<void(bool success, const QJsonObject& result, const QString& error)>;

    explicit Service(QObject* parent = nullptr);

    QString serverUrl() const;
    bool connected() const;

    void setServerUrl(const QString& url);

    void connectToSocket();
    void disconnectFromSocket();

    void request(const ::Common::Communication::WebSocket::Method& method, const QJsonObject& params, ResponseCallback callback);
    void publish(const ::Common::Communication::WebSocket::Topic& topic, const QJsonObject& params = QJsonObject());
    void subscribe(const ::Common::Communication::WebSocket::Topic& topic);
    void unsubscribe(const ::Common::Communication::WebSocket::Topic& topic);

  signals:
    void serverUrlChanged();
    void connectedChanged();
    void publishReceived(const ::Common::Communication::WebSocket::Topic& topic, const QJsonObject& data);

  private:
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError error);
    void onTextMessageReceived(const QString& message);
    void dispatchMessage(const QJsonObject& message);
    void resubscribeAll();

    QWebSocket m_webSocket;
    QString m_serverUrl;
    bool m_connected;
    int m_nextRequestId;
    QHash<QString, ResponseCallback> m_pendingRequests;
    QList<::Common::Communication::WebSocket::Topic> m_subscribedTopics;
};

} // namespace Common::Communication::WebSocket::Client

#endif // COMMON_COMMUNICATION_WEBSOCKET_CLIENT_SERVICE_H