#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "ARLifeCycleComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UARLifeCycleComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UARLifeCycleComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
