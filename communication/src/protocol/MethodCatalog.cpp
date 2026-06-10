#include "MethodCatalog.h"

namespace Common::Communication::Protocol
{

QList<Common::Communication::WebSocket::Method> MethodCatalog::allMethods()
{
    using Common::Communication::WebSocket::Method;

    return {
        Method::Subscribe,
        Method::Unsubscribe,
        Method::SetBrightness,
        Method::SetVolume,
        Method::SetDeviceId,
        Method::UpdateSystemConfig,
        Method::AddApp,
        Method::UpdateApp,
        Method::RemoveApp,
        Method::GetStatus,
        Method::GetProcessorTemperature,
        Method::GetMedia,
        Method::GetAllMedia,
        Method::GetEnvironment,
        Method::Shutdown,
        Method::Reboot,
        Method::PlaySound,
        Method::StopSound,
    };
}

QList<Common::Communication::WebSocket::Topic> MethodCatalog::allTopics()
{
    using Common::Communication::WebSocket::Topic;

    return {
        Topic::Configuration,
        Topic::ApplicationList,
        Topic::ApplicationDetail,
        Topic::Media,
        Topic::ApplicationStatus,
        Topic::ProcessorTemperature,
        Topic::BackendStatus,
        Topic::Environment,
    };
}

} // namespace Common::Communication::Protocol
