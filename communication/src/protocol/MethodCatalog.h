#ifndef COMMON_COMMUNICATION_PROTOCOL_METHODCATALOG_H
#define COMMON_COMMUNICATION_PROTOCOL_METHODCATALOG_H

#include <QList>

#include "websocket/Types.h"

namespace Common::Communication::Protocol
{

class MethodCatalog
{
  public:
    static QList<Common::Communication::WebSocket::Method> allMethods();
    static QList<Common::Communication::WebSocket::Topic> allTopics();
};

} // namespace Common::Communication::Protocol

#endif // COMMON_COMMUNICATION_PROTOCOL_METHODCATALOG_H
