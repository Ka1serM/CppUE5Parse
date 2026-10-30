#pragma once

#include "ue5parse/runtime/Engine/Components/LocalLightComponent.h"

#include "PointLightComponent.generated.h"

namespace ue5parse::runtime {

// UPointLightComponent: a light that shines in all directions from a sphere or a capsule (UE's
// Components/PointLightComponent.h).
UCLASS()
class UPointLightComponent : public ULocalLightComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bUseInverseSquaredFalloff = true;
    UPROPERTY()
    float LightFalloffExponent = 8.0f;
    UPROPERTY()
    float SourceRadius = 0;
    UPROPERTY()
    float SoftSourceRadius = 0;
    UPROPERTY()
    float SourceLength = 0;

    void Serialize(FArchive& Ar) override;
    float ComputeLightBrightness() const override;
};

}
