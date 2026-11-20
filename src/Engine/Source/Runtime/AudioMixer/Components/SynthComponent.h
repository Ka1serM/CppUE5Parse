#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "SynthComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USynthComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit USynthComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
