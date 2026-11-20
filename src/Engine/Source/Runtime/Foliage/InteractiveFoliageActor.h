#pragma once

#include "Engine/Source/Runtime/Engine/Engine/StaticMeshActor.h"

#include "InteractiveFoliageActor.generated.h"

namespace ue5parse::runtime {

class UCapsuleComponent;

UCLASS()
class AInteractiveFoliageActor : public AStaticMeshActor {
    GENERATED_BODY()
public:
    explicit AInteractiveFoliageActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UCapsuleComponent> CapsuleComponent = nullptr;
    UPROPERTY()
    float FoliageDamageImpulseScale = 20.0f;
    UPROPERTY()
    float FoliageTouchImpulseScale = 10.0f;
    UPROPERTY()
    float FoliageStiffness = 10.0f;
    UPROPERTY()
    float FoliageStiffnessQuadratic = 0.3f;
    UPROPERTY()
    float FoliageDamping = 2.0f;
    UPROPERTY()
    float MaxDamageImpulse = 100000.0f;
    UPROPERTY()
    float MaxTouchImpulse = 1000.0f;
    UPROPERTY()
    float MaxForce = 100000.0f;
    UPROPERTY()
    float Mass = 1.0f;
};

}
