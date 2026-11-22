#include "Engine/Source/Runtime/Engine/Engine/DemoNetDriver.h"

#include "Engine/Source/Runtime/Engine/Engine/NetConnection.h"

namespace ue5parse::runtime {

bool UDemoNetDriver::IsServer() const {
    return ServerConnection == nullptr || IsRecording();
}

bool UDemoNetDriver::IsRecording() const {
    return ClientConnections.Num() > 0 && ClientConnections[0] != nullptr && ClientConnections[0]->GetConnectionState() != USOCK_Closed;
}

bool UDemoNetDriver::IsPlaying() const {
    return ServerConnection != nullptr && ServerConnection->GetConnectionState() != USOCK_Closed;
}

}
