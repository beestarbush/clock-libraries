#ifndef COMMON_COMMUNICATION_WEBSOCKET_SUBSCRIPTIONHELPER_H
#define COMMON_COMMUNICATION_WEBSOCKET_SUBSCRIPTIONHELPER_H

#include <QMetaObject>
#include <QObject>
#include <QTimer>

#include <functional>

#include "websocket/client/Service.h"

namespace Common::Communication::WebSocket
{

class SubscriptionHelper : public QObject
{
    Q_OBJECT

  public:
    using DataHandler = std::function<void(const QJsonObject& data)>;

    explicit SubscriptionHelper(Client::Service& webSocket, QObject* parent = nullptr);

    void setStartupTimeoutMs(int timeoutMs);
    void start(const Topic& topic, Method bootstrapMethod, DataHandler handler);
    void stop();

  signals:
    void startupCompleted();
    void startupTimedOut();

  private:
    void performBootstrap();
    void completeStartup();

    Client::Service& m_webSocket;
    QTimer m_startupTimeoutTimer;
    QMetaObject::Connection m_connectionWatcher;

    bool m_startupInProgress;
    Topic m_topic;
    Method m_bootstrapMethod;
    DataHandler m_handler;
};

} // namespace Common::Communication::WebSocket

#endif // COMMON_COMMUNICATION_WEBSOCKET_SUBSCRIPTIONHELPER_H
