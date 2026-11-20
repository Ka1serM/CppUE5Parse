#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "FoundObjectComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UFoundObjectComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UFoundObjectComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
