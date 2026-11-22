#include "Engine/Source/Runtime/Engine/Engine/NetDriver.h"

#include "Engine/Source/Runtime/Core/CoreGlobals.h"
#include "Engine/Source/Runtime/Engine/Engine/NetConnection.h"

namespace ue5parse::runtime {

ENetMode UNetDriver::GetNetMode() const {
    if (!IsServer())
        return NM_Client;
    return GIsClient ? NM_ListenServer : NM_DedicatedServer;
}

bool UNetDriver::IsServer() const {
    return ServerConnection == nullptr;
}

}
