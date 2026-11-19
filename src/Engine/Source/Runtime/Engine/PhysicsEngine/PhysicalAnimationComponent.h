#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "PhysicalAnimationComponent.generated.h"

namespace ue5parse::runtime {

class USkeletalMeshComponent;

USTRUCT()
struct FPhysicalAnimationData {
    GENERATED_BODY()
    UPROPERTY()
    FName BodyName;
    UPROPERTY()
    bool bIsLocalSimulation = true;
    UPROPERTY()
    float OrientationStrength = 0.0f;
    UPROPERTY()
    float AngularVelocityStrength = 0.0f;
    UPROPERTY()
    float PositionStrength = 0.0f;
    UPROPERTY()
    float VelocityStrength = 0.0f;
    UPROPERTY()
    float MaxLinearForce = 0.0f;
    UPROPERTY()
    float MaxAngularForce = 0.0f;
};

UCLASS()
class UPhysicalAnimationComponent : public UActorComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    float StrengthMultiplyer = 1.0f;
    UPROPERTY()
    TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent = nullptr;
};

}
