#pragma once

#include "Engine/Source/Runtime/Engine/Components/LocalLightComponent.h"

#include "PointLightComponent.generated.h"

namespace ue5parse::runtime {

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
