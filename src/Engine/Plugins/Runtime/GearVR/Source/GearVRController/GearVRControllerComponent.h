#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "GearVRControllerComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UGearVRControllerComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UGearVRControllerComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
