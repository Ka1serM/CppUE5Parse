#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "ISMPoolComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UISMPoolComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UISMPoolComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
