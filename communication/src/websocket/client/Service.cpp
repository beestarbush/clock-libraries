#include "websocket/client/Service.h"

#include <QJsonDocument>
#include <QLoggingCategory>

#include "websocket/Frame.h"

Q_LOGGING_CATEGORY(WebSocketClientService, "WebSocketClient")

namespace Common::Communication::WebSocket::Client
{

const QString PROPERTY_SERVER_URL_DEFAULT = QStringLiteral("ws://127.0.0.1:5000/ws");

namespace
{
QString requestIdKey(const QJsonValue& idValue)
{
    if (idValue.isString()) {
        return idValue.toString();
    }
    if (idValue.isDouble()) {
        return QString::number(idValue.toDouble(), 'g', 16);
    }

    return QString();
}
} // namespace

Service::Service(QObject* parent)
    : QObject(parent),
      m_webSocket(QString(), QWebSocketProtocol::VersionLatest, this),
      m_serverUrl(PROPERTY_SERVER_URL_DEFAULT),
      m_connected(false),
      m_nextRequestId(1)
{
    connect(&m_webSocket, &QWebSocket::connected, this, &Service::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &Service::onDisconnected);
    connect(&m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::errorOccurred), this, &Service::onError);
    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &Service::onTextMessageReceived);
}

QString Service::serverUrl() const
{
    return m_serverUrl;
}

bool Service::connected() const
{
    return m_connected;
}

void Service::setServerUrl(const QString& url)
{
    if (m_serverUrl == url) {
        return;
    }

    m_serverUrl = url;
    emit serverUrlChanged();
}

void Service::connectToSocket()
{
    if (m_serverUrl.isEmpty()) {
        return;
    }
    if (m_webSocket.state() == QAbstractSocket::ConnectedState ||
        m_webSocket.state() == QAbstractSocket::ConnectingState) {
        return;
    }

    qCDebug(WebSocketClientService) << "Connecting to" << m_serverUrl;
    m_webSocket.open(QUrl(m_serverUrl));
}

void Service::disconnectFromSocket()
{
    m_webSocket.close();
}

void Service::request(const ::Common::Communication::WebSocket::Method& method, const QJsonObject& params, ResponseCallback callback)
{
    if (!m_connected) {
        qCWarning(WebSocketClientService) << "Disconnected, cannot send request:" << methodToString(method);
        if (callback) {
            callback(false, QJsonObject(), QStringLiteral("Disconnected"));
        }
        return;
    }

    const QJsonValue id = m_nextRequestId++;
    const QString idKey = requestIdKey(id);
    if (idKey.isEmpty() || !Frame::isValidId(id)) {
        qCWarning(WebSocketClientService) << "Generated invalid request id";
        if (callback) {
            callback(false, QJsonObject(), QStringLiteral("Invalid request id"));
        }
        return;
    }

    m_pendingRequests.insert(idKey, callback);

    QJsonObject req = Frame::buildRequest(id, method, params);
    QString message = QString::fromUtf8(QJsonDocument(req).toJson(QJsonDocument::Compact));
    qCDebug(WebSocketClientService) << "-> request" << message;
    m_webSocket.sendTextMessage(message);
}

void Service::publish(const ::Common::Communication::WebSocket::Topic& topic, const QJsonObject& params)
{
    if (!m_connected) {
        qCWarning(WebSocketClientService) << "Disconnected, cannot publish to topic:" << topic;
        return;
    }

    QJsonObject msg = Frame::buildPublish(topic, params);
    QString message = QString::fromUtf8(QJsonDocument(msg).toJson(QJsonDocument::Compact));
    qCDebug(WebSocketClientService) << "-> publish" << message;
    m_webSocket.sendTextMessage(message);
}

void Service::subscribe(const ::Common::Communication::WebSocket::Topic& topic)
{
    if (!m_subscribedTopics.contains(topic)) {
        m_subscribedTopics.append(topic);
    }

    if (!m_connected) {
        qCDebug(WebSocketClientService) << "Will subscribe to" << topic << "when connected";
        return;
    }

    QJsonObject params;
    params["topic"] = topicToString(topic);
    request(::Common::Communication::WebSocket::Method::Subscribe, params, [topic](bool success, const QJsonObject&, const QString& error) {
        if (success) {
            qCDebug(WebSocketClientService) << "Subscribed to topic:" << topic;
        }
        else {
            qCWarning(WebSocketClientService) << "Failed to subscribe to" << topic << ":" << error;
        }
    });
}

void Service::unsubscribe(const ::Common::Communication::WebSocket::Topic& topic)
{
    m_subscribedTopics.removeAll(topic);

    if (!m_connected) {
        return;
    }

    QJsonObject params;
    params["topic"] = topicToString(topic);
    request(::Common::Communication::WebSocket::Method::Unsubscribe, params, [topic](bool success, const QJsonObject&, const QString& error) {
        if (success) {
            qCDebug(WebSocketClientService) << "Unsubscribed from topic:" << topic;
        }
        else {
            qCWarning(WebSocketClientService) << "Failed to unsubscribe from" << topic << ":" << error;
        }
    });
}

void Service::onConnected()
{
    qCDebug(WebSocketClientService) << "Connected";
    m_connected = true;
    emit connectedChanged();
    resubscribeAll();
}

void Service::onDisconnected()
{
    qCDebug(WebSocketClientService) << "Disconnected";

    for (auto it = m_pendingRequests.begin(); it != m_pendingRequests.end(); ++it) {
        if (it.value()) {
            it.value()(false, QJsonObject(), QStringLiteral("Disconnected"));
        }
    }
    m_pendingRequests.clear();

    if (m_connected) {
        m_connected = false;
        emit connectedChanged();
    }
}

void Service::onError(QAbstractSocket::SocketError error)
{
    qCWarning(WebSocketClientService) << "Error:" << error << m_webSocket.errorString();
}

void Service::onTextMessageReceived(const QString& message)
{
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
        qCWarning(WebSocketClientService) << "Received invalid JSON:" << message;
        return;
    }

    dispatchMessage(doc.object());
}

void Service::dispatchMessage(const QJsonObject& message)
{
    QString messageStr = QString::fromUtf8(QJsonDocument(message).toJson(QJsonDocument::Compact));
    qCDebug(WebSocketClientService) << "<- response/publish" << messageStr;

    if (Frame::isResponse(message)) {
        const QString id = requestIdKey(Frame::parseId(message));
        if (id.isEmpty()) {
            qCWarning(WebSocketClientService) << "Received response with invalid request id";
            return;
        }

        if (!m_pendingRequests.contains(id)) {
            qCDebug(WebSocketClientService) << "Received response for unknown request id:" << id;
            return;
        }

        ResponseCallback callback = m_pendingRequests.take(id);
        QString error = Frame::parseErrorMessage(message);
        if (!error.isEmpty()) {
            callback(false, QJsonObject(), error);
            return;
        }

        callback(true, Frame::parseResult(message), QString());
        return;
    }

    if (Frame::isPublish(message)) {
        emit publishReceived(Frame::parseTopic(message), Frame::parseParams(message));
        return;
    }

    qCWarning(WebSocketClientService) << "Unknown message type received";
}

void Service::resubscribeAll()
{
    for (const ::Common::Communication::WebSocket::Topic& topic : m_subscribedTopics) {
        QJsonObject params;
        params["topic"] = topicToString(topic);
        request(::Common::Communication::WebSocket::Method::Subscribe, params, [topic](bool success, const QJsonObject&, const QString& error) {
            if (success) {
                qCDebug(WebSocketClientService) << "Subscribed to topic:" << topicToString(topic);
            }
            else {
                qCWarning(WebSocketClientService) << "Failed to subscribe to" << topicToString(topic) << ":" << error;
            }
        });
    }
}

} // namespace Common::Communication::WebSocket::Client