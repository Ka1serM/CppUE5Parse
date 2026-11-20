#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "LinearTimecodeComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ULinearTimecodeComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit ULinearTimecodeComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
