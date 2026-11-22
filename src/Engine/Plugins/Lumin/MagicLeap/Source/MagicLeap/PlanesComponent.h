#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "PlanesComponent.generated.h"

namespace ue5parse::runtime {

class UBoxComponent;

UCLASS()
class UPlanesComponent : public USceneComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UBoxComponent> SearchVolume = nullptr;

    explicit UPlanesComponent(const FObjectInitializer& ObjectInitializer);
};

}
