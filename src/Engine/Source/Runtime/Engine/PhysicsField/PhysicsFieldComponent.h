#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "PhysicsFieldComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UPhysicsFieldComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UPhysicsFieldComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
