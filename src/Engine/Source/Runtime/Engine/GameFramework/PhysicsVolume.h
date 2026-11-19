#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Volume.h"

#include "PhysicsVolume.generated.h"

namespace ue5parse::runtime {

UCLASS()
class APhysicsVolume : public AVolume {
    GENERATED_BODY()
public:
    UPROPERTY()
    float TerminalVelocity = 4000.0f;
    UPROPERTY()
    int32 Priority = 0;
    UPROPERTY()
    float FluidFriction = 0.3f;
    UPROPERTY()
    uint8 bWaterVolume : 1 = false;
    UPROPERTY()
    uint8 bPhysicsOnContact : 1 = false;

    explicit APhysicsVolume(const FObjectInitializer& ObjectInitializer);
};

}
