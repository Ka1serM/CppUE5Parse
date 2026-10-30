#pragma once

#include "ue5parse/runtime/Engine/GameFramework/Actor.h"

#include "Info.generated.h"

namespace ue5parse::runtime {

// AInfo: an actor without a place in the world, for managers and state (UE's GameFramework/Info.h).
UCLASS(Abstract)
class AInfo : public AActor {
    GENERATED_BODY()
};

}
