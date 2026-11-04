#pragma once

#include "Engine/Source/Runtime/Engine/Components/LightComponentBase.h"

#include "SkyLightComponent.generated.h"

namespace ue5parse::runtime {

class UTextureCube;

enum ESkyLightSourceType : int32 { SLS_CapturedScene, SLS_SpecifiedCubemap };

UCLASS()
class USkyLightComponent : public ULightComponentBase {
    GENERATED_BODY()
public:
    explicit USkyLightComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TEnumAsByte<ESkyLightSourceType> SourceType = SLS_CapturedScene;
    UPROPERTY()
    TObjectPtr<UTextureCube> Cubemap = nullptr;
    UPROPERTY()
    float SourceCubemapAngle = 0;
    UPROPERTY()
    bool bLowerHemisphereIsBlack = true;
    UPROPERTY()
    FLinearColor LowerHemisphereColor = FLinearColor(0, 0, 0, 1);
};

}
