#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/MovementComponent.h"

#include "RotatingMovementComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class URotatingMovementComponent : public UMovementComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    FRotator RotationRate = FRotator(0.0f, 180.0f, 0.0f);
    UPROPERTY()
    FVector PivotTranslation = FVector::ZeroVector;
    UPROPERTY()
    uint32 bRotationInLocalSpace : 1 = true;

    explicit URotatingMovementComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
        bAutoRegisterPhysicsVolumeUpdates = false;
        bComponentShouldUpdatePhysicsVolume = false;
    }
};

}
