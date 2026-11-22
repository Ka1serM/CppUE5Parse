#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "MagicLeapPlanesComponent.generated.h"

namespace ue5parse::runtime {

class UBoxComponent;

UCLASS()
class UMagicLeapPlanesComponent : public USceneComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UBoxComponent> SearchVolume = nullptr;

    explicit UMagicLeapPlanesComponent(const FObjectInitializer& ObjectInitializer);
};

}
