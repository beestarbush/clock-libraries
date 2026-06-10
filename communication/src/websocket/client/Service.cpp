#include "websocket/client/Service.h"

#include <QJsonDocument>
#include <QLoggingCategory>
#include <QNetworkProxy>

#include "websocket/Frame.h"

Q_LOGGING_CATEGORY(WebSocketClientService, "WebSocketClient")

namespace Common::Communication::WebSocket::Client
{

const QString PROPERTY_SERVER_URL_DEFAULT = QStringLiteral("ws://127.0.0.1:5000/ws");
constexpr int RECONNECT_INTERVAL_MS = 2000;

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
      m_nextRequestId(1),
      m_autoReconnectEnabled(true)
{
    // Keep local backend connections deterministic on embedded devices.
    // System proxy auto-discovery may introduce multi-second delays.
    m_webSocket.setProxy(QNetworkProxy::NoProxy);

    connect(&m_webSocket, &QWebSocket::connected, this, &Service::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &Service::onDisconnected);
    connect(&m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::errorOccurred), this, &Service::onError);
    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &Service::onTextMessageReceived);

    m_reconnectTimer.setSingleShot(true);
    m_reconnectTimer.setInterval(RECONNECT_INTERVAL_MS);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &Service::connectToSocket);
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
    m_autoReconnectEnabled = true;

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
    m_autoReconnectEnabled = false;
    m_reconnectTimer.stop();
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

    subscribeInternal(topic, QJsonObject(), [topic](bool success, const QJsonObject&, const QString& error) {
        if (success) {
            qCDebug(WebSocketClientService) << "Subscribed to topic:" << topic;
        }
        else {
            qCWarning(WebSocketClientService) << "Failed to subscribe to" << topic << ":" << error;
        }
    });
}

void Service::subscribe(const ::Common::Communication::WebSocket::Topic& topic,
                        const QJsonObject& additionalParams,
                        QObject* owner,
                        PublishCallback onPublish,
                        ResponseCallback onAck)
{
    bool updated = false;
    for (ParamSubscription& subscription : m_paramSubscriptions) {
        if (subscription.topic == topic && subscription.additionalParams == additionalParams) {
            subscription.owner = owner;
            subscription.onPublish = onPublish;
            updated = true;
            break;
        }
    }

    if (!updated) {
        m_paramSubscriptions.append(ParamSubscription{topic, additionalParams, owner, onPublish});
    }

    if (!m_connected) {
        qCDebug(WebSocketClientService) << "Will subscribe to" << topic << "with params when connected";
        return;
    }

    subscribeInternal(topic, additionalParams, onAck);
}

void Service::unsubscribe(const ::Common::Communication::WebSocket::Topic& topic)
{
    m_subscribedTopics.removeAll(topic);

    if (!m_connected) {
        return;
    }

    QJsonObject params = buildSubscribeParams(topic, QJsonObject());
    request(::Common::Communication::WebSocket::Method::Unsubscribe, params, [topic](bool success, const QJsonObject&, const QString& error) {
        if (success) {
            qCDebug(WebSocketClientService) << "Unsubscribed from topic:" << topic;
        }
        else {
            qCWarning(WebSocketClientService) << "Failed to unsubscribe from" << topic << ":" << error;
        }
    });
}

void Service::unsubscribe(const ::Common::Communication::WebSocket::Topic& topic,
                          const QJsonObject& additionalParams,
                          ResponseCallback onAck)
{
    for (auto it = m_paramSubscriptions.begin(); it != m_paramSubscriptions.end();) {
        if (it->topic == topic && it->additionalParams == additionalParams) {
            it = m_paramSubscriptions.erase(it);
        }
        else {
            ++it;
        }
    }

    if (!m_connected) {
        return;
    }

    QJsonObject params = buildSubscribeParams(topic, additionalParams);
    request(::Common::Communication::WebSocket::Method::Unsubscribe, params, onAck);
}

void Service::onConnected()
{
    qCDebug(WebSocketClientService) << "Connected";
    m_reconnectTimer.stop();
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

    scheduleReconnect();
}

void Service::onError(QAbstractSocket::SocketError error)
{
    qCWarning(WebSocketClientService) << "Error:" << error << m_webSocket.errorString();

    if (m_webSocket.state() == QAbstractSocket::UnconnectedState) {
        scheduleReconnect();
    }
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
        if (!callback) {
            return;
        }

        QString error = Frame::parseErrorMessage(message);
        if (!error.isEmpty()) {
            callback(false, QJsonObject(), error);
            return;
        }

        callback(true, Frame::parseResult(message), QString());
        return;
    }

    if (Frame::isPublish(message)) {
        const ::Common::Communication::WebSocket::Topic topic = Frame::parseTopic(message);
        const QJsonObject data = Frame::parseParams(message);

        // Prune stale owner-bound subscriptions first.
        for (auto it = m_paramSubscriptions.begin(); it != m_paramSubscriptions.end();) {
            if (it->owner.isNull()) {
                it = m_paramSubscriptions.erase(it);
                continue;
            }

            ++it;
        }

        // Dispatch using a snapshot because callbacks may subscribe/unsubscribe.
        QList<PublishCallback> callbacks;
        callbacks.reserve(m_paramSubscriptions.size());

        for (const ParamSubscription& subscription : m_paramSubscriptions) {
            if (subscription.topic != topic) {
                continue;
            }
            if (!paramsMatch(subscription.additionalParams, data)) {
                continue;
            }
            if (subscription.onPublish) {
                callbacks.append(subscription.onPublish);
            }
        }

        for (const PublishCallback& callback : callbacks) {
            callback(data);
        }

        emit publishReceived(topic, data);
        return;
    }

    qCWarning(WebSocketClientService) << "Unknown message type received";
}

void Service::resubscribeAll()
{
    for (const ::Common::Communication::WebSocket::Topic& topic : m_subscribedTopics) {
        subscribeInternal(topic, QJsonObject(), [topic](bool success, const QJsonObject&, const QString& error) {
            if (success) {
                qCDebug(WebSocketClientService) << "Subscribed to topic:" << topicToString(topic);
            }
            else {
                qCWarning(WebSocketClientService) << "Failed to subscribe to" << topicToString(topic) << ":" << error;
            }
        });
    }

    for (auto it = m_paramSubscriptions.begin(); it != m_paramSubscriptions.end();) {
        if (it->owner.isNull()) {
            it = m_paramSubscriptions.erase(it);
            continue;
        }

        subscribeInternal(it->topic, it->additionalParams, {});
        ++it;
    }
}

QJsonObject Service::buildSubscribeParams(const ::Common::Communication::WebSocket::Topic& topic,
                                          const QJsonObject& additionalParams) const
{
    QJsonObject params = additionalParams;
    params["topic"] = topicToString(topic);
    return params;
}

bool Service::paramsMatch(const QJsonObject& expectedParams, const QJsonObject& data) const
{
    for (auto it = expectedParams.constBegin(); it != expectedParams.constEnd(); ++it) {
        if (data.value(it.key()) != it.value()) {
            return false;
        }
    }
    return true;
}

void Service::subscribeInternal(const ::Common::Communication::WebSocket::Topic& topic,
                                const QJsonObject& additionalParams,
                                ResponseCallback onAck)
{
    QJsonObject params = buildSubscribeParams(topic, additionalParams);
    request(::Common::Communication::WebSocket::Method::Subscribe, params, onAck);
}

void Service::scheduleReconnect()
{
    if (!m_autoReconnectEnabled || m_serverUrl.isEmpty() || m_reconnectTimer.isActive()) {
        return;
    }

    qCDebug(WebSocketClientService) << "Scheduling reconnect in" << RECONNECT_INTERVAL_MS << "ms";
    m_reconnectTimer.start();
}

} // namespace Common::Communication::WebSocket::Client