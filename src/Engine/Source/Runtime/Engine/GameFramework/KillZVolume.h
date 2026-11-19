#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/PhysicsVolume.h"

#include "KillZVolume.generated.h"

namespace ue5parse::runtime {

UCLASS()
class AKillZVolume : public APhysicsVolume {
    GENERATED_BODY()
public:
    explicit AKillZVolume(const FObjectInitializer& ObjectInitializer);
};

}
