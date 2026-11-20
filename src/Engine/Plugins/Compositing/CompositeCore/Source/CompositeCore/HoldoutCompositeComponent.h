#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "HoldoutCompositeComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UHoldoutCompositeComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UHoldoutCompositeComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
