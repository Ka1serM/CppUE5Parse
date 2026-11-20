#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "MagicLeapARPinComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMagicLeapARPinComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UMagicLeapARPinComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
