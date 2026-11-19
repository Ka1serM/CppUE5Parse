#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "LightmassPortalComponent.generated.h"

namespace ue5parse::runtime {

class UBoxComponent;

UCLASS()
class ULightmassPortalComponent : public USceneComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UBoxComponent> PreviewBox = nullptr;

    explicit ULightmassPortalComponent(const FObjectInitializer& ObjectInitializer);
};

}
