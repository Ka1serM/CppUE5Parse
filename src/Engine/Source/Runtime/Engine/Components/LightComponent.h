#pragma once

#include "Engine/Source/Runtime/Engine/Components/LightComponentBase.h"
#include "Engine/Source/Runtime/Engine/Engine/Scene.h"

#include "LightComponent.generated.h"

namespace ue5parse::runtime {

class UTextureLightProfile;

UCLASS(Abstract)
class ULightComponent : public ULightComponentBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    float Temperature = 6500.0f;
    UPROPERTY()
    bool bUseTemperature = false;
    UPROPERTY()
    TObjectPtr<UTextureLightProfile> IESTexture = nullptr;
    UPROPERTY()
    bool bUseIESBrightness = false;
    UPROPERTY()
    float IESBrightnessScale = 1.0f;

    void PostLoad() override;
    virtual ELightUnits GetLightUnits() const;
    virtual float ComputeLightBrightness() const;
};

}
