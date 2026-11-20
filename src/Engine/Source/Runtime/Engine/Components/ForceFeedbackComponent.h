#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "ForceFeedbackComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UForceFeedbackComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UForceFeedbackComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
