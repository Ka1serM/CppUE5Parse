#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "PhysicsThrusterComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UPhysicsThrusterComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UPhysicsThrusterComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
