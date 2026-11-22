#pragma once

#include "Engine/Source/Runtime/Engine/Components/SkyAtmosphereComponent.h"

#include "AtmosphericFogComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UAtmosphericFogComponent : public USkyAtmosphereComponent {
    GENERATED_BODY()
public:
    explicit UAtmosphericFogComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override;
};

}
