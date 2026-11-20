#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "SkyAtmosphereComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USkyAtmosphereComponent : public USceneComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    FGuid bStaticLightingBuiltGUID;

    explicit USkyAtmosphereComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override;
};

}
