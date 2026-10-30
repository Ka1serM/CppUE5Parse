#pragma once

#include "ue5parse/runtime/Engine/Components/SceneComponent.h"

#include "LightComponentBase.generated.h"

namespace ue5parse::runtime {

// ULightComponentBase: the color and intensity of every light, sky lights included (UE's
// Components/LightComponentBase.h).
UCLASS(Abstract)
class ULightComponentBase : public USceneComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    float Intensity = 3.1415926535897932f;
    UPROPERTY()
    FColor LightColor = FColor::White;
    UPROPERTY()
    bool bAffectsWorld = true;
    UPROPERTY()
    bool CastShadows = true;

    void Serialize(FArchive& Ar) override;
    FLinearColor GetLightColor() const { return FLinearColor(LightColor); }
};

}
