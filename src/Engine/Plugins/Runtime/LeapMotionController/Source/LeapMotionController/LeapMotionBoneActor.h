#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "LeapMotionBoneActor.generated.h"

namespace ue5parse::runtime {

class UPrimitiveComponent;
class UStaticMeshComponent;

UCLASS()
class ALeapMotionBoneActor : public AActor {
    GENERATED_BODY()
public:
    explicit ALeapMotionBoneActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UPrimitiveComponent> PrimitiveComponent = nullptr;
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> StaticMeshComponent = nullptr;
};

}
