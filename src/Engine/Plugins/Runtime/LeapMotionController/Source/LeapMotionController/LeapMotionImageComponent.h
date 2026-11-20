#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "LeapMotionImageComponent.generated.h"

namespace ue5parse::runtime {

class UStaticMeshComponent;

UCLASS()
class ULeapMotionImageComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit ULeapMotionImageComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> DisplaySurfaceComponent = nullptr;
};

}
