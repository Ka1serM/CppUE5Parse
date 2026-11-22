#pragma once

#include "Engine/Source/Runtime/Engine/Engine/NetConnection.h"

#include "DemoNetConnection.generated.h"

namespace ue5parse::runtime {

UCLASS(Transient)
class UDemoNetConnection : public UNetConnection {
    GENERATED_BODY()
};

}
