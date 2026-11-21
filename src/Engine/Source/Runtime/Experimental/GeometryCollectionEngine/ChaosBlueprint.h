#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "ChaosBlueprint.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UChaosDestructionListener : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UChaosDestructionListener(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
