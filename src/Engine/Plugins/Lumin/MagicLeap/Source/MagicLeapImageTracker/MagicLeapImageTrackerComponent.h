#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "MagicLeapImageTrackerComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMagicLeapImageTrackerComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UMagicLeapImageTrackerComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
