#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "NetConnection.generated.h"

namespace ue5parse::runtime {

enum EConnectionState {
    USOCK_Invalid = 0,
    USOCK_Closed = 1,
    USOCK_Pending = 2,
    USOCK_Open = 3,
    USOCK_Closing = 4,
};

UCLASS(Abstract, Transient)
class UNetConnection : public UObject {
    GENERATED_BODY()
public:
    EConnectionState GetConnectionState() const { return State; }
    void SetConnectionState(EConnectionState ConnectionState) { State = ConnectionState; }
    bool IsClosingOrClosed() const { return State == USOCK_Closing || State == USOCK_Closed; }

private:
    EConnectionState State = USOCK_Invalid;
};

}
