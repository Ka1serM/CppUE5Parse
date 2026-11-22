#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "WidgetInteractionComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UWidgetInteractionComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UWidgetInteractionComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
