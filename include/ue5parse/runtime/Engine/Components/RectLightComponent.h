#pragma once

#include "ue5parse/runtime/Engine/Components/LocalLightComponent.h"

#include "RectLightComponent.generated.h"

namespace ue5parse::runtime {

class UTexture;

// URectLightComponent: a light from a rectangle with barn doors (UE's Components/RectLightComponent.h).
UCLASS()
class URectLightComponent : public ULocalLightComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    float SourceWidth = 64.0f;
    UPROPERTY()
    float SourceHeight = 64.0f;
    UPROPERTY()
    float BarnDoorAngle = 88.0f;
    UPROPERTY()
    float BarnDoorLength = 20.0f;
    UPROPERTY()
    UTexture* SourceTexture = nullptr;

    float ComputeLightBrightness() const override;
};

}
