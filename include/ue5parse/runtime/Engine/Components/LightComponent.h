#pragma once

#include "ue5parse/runtime/Engine/Components/LightComponentBase.h"
#include "ue5parse/runtime/Engine/Engine/Scene.h"

#include "LightComponent.generated.h"

namespace ue5parse::runtime {

class UTextureLightProfile;

// ULightComponent: a light that the renderer draws, with its temperature and IES profile (UE's
// Components/LightComponent.h). A light shines along its component's +X axis.
UCLASS(Abstract)
class ULightComponent : public ULightComponentBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    float Temperature = 6500.0f;
    UPROPERTY()
    bool bUseTemperature = false;
    UPROPERTY()
    UTextureLightProfile* IESTexture = nullptr;
    UPROPERTY()
    bool bUseIESBrightness = false;
    UPROPERTY()
    float IESBrightnessScale = 1.0f;

    void PostLoad() override;
    virtual ELightUnits GetLightUnits() const;
    // The brightness the renderer uses, in UE's internal units.
    virtual float ComputeLightBrightness() const;
    // The linear light color times the brightness, times the color of the temperature when bUseTemperature is set.
    FLinearColor GetColoredLightBrightness() const;
};

}
