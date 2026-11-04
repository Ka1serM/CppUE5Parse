#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "LightComponentBase.generated.h"

namespace ue5parse::runtime {

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
    UPROPERTY()
    float IndirectLightingIntensity = 1.0f;

    void Serialize(FArchive& Ar) override;
};

}
