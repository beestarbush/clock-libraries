#ifndef COMMON_COMMUNICATION_WEBSOCKET_FRAME_H
#define COMMON_COMMUNICATION_WEBSOCKET_FRAME_H

#include <QJsonObject>
#include <QJsonValue>
#include <QString>

#include "Types.h"

namespace Common::Communication::WebSocket
{

class Frame
{
  public:
    static QJsonObject buildRequest(const QJsonValue& id, Method method, const QJsonObject& params = QJsonObject());
    static QJsonObject buildResponse(const QJsonValue& id, const QJsonObject& result = QJsonObject());
    static QJsonObject buildErrorResponse(const QJsonValue& id, int code, const QString& message);
    static QJsonObject buildPublish(Topic topic, const QJsonObject& params = QJsonObject());

    static bool isRequest(const QJsonObject& message);
    static bool isResponse(const QJsonObject& message);
    static bool isPublish(const QJsonObject& message);

    static QJsonValue parseId(const QJsonObject& message);
    static bool isValidId(const QJsonValue& id);
    static Method parseMethod(const QJsonObject& message);
    static Topic parseTopic(const QJsonObject& message);
    static QJsonObject parseParams(const QJsonObject& message);
    static QJsonObject parseResult(const QJsonObject& message);
    static QString parseErrorMessage(const QJsonObject& message);
};

} // namespace Common::Communication::WebSocket

#endif // COMMON_COMMUNICATION_WEBSOCKET_FRAME_H
