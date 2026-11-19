#pragma once

#include "Engine/Source/Runtime/Engine/Components/LocalLightComponent.h"

#include "PointLightComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UPointLightComponent : public ULocalLightComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    uint8 bUseInverseSquaredFalloff : 1 = true;
    UPROPERTY()
    float LightFalloffExponent = 8.0f;
    UPROPERTY()
    float SourceRadius = 0.0f;
    UPROPERTY()
    float SoftSourceRadius = 0.0f;
    UPROPERTY()
    float SourceLength = 0.0f;

    void Serialize(FArchive& Ar) override;
    float ComputeLightBrightness() const override;
};

}
