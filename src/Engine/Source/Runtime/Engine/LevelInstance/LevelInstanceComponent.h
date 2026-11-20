#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "LevelInstanceComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ULevelInstanceComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit ULevelInstanceComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
