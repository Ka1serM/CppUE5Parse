#pragma once

#include "Engine/Source/Runtime/Engine/Components/LocalLightComponent.h"

#include "RectLightComponent.generated.h"

namespace ue5parse::runtime {

class UTexture;

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
    float LightFunctionConeAngle = 0.0f;
    UPROPERTY()
    TObjectPtr<UTexture> SourceTexture = nullptr;
    UPROPERTY()
    FVector2f SourceTextureScale = FVector2f(1.0f, 1.0f);
    UPROPERTY()
    FVector2f SourceTextureOffset = FVector2f(0.0f, 0.0f);
    UPROPERTY()
    bool bLightRequiresBrokenEVMath = false;

    float ComputeLightBrightness() const override;
};

}
