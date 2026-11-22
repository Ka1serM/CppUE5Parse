#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "LocalHeightFogComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ULocalHeightFogComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit ULocalHeightFogComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
