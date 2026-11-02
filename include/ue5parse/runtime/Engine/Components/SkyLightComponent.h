#pragma once

#include "ue5parse/runtime/Engine/Components/LightComponentBase.h"

#include "SkyLightComponent.generated.h"

namespace ue5parse::runtime {

class UTextureCube;

enum ESkyLightSourceType : int32 { SLS_CapturedScene, SLS_SpecifiedCubemap };

// USkyLightComponent: the light of the distant scene or of a cube from every direction (UE's
// Components/SkyLightComponent.h). The members that decide its light.
UCLASS()
class USkyLightComponent : public ULightComponentBase {
    GENERATED_BODY()
public:
    explicit USkyLightComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TEnumAsByte<ESkyLightSourceType> SourceType = SLS_CapturedScene;
    UPROPERTY()
    TObjectPtr<UTextureCube> Cubemap = nullptr;
    // Degrees about +Z that the cube is looked up at.
    UPROPERTY()
    float SourceCubemapAngle = 0;
    // The light from below the horizon goes to LowerHemisphereColor by its alpha.
    UPROPERTY()
    bool bLowerHemisphereIsBlack = true;
    UPROPERTY()
    FLinearColor LowerHemisphereColor = FLinearColor(0, 0, 0, 1);
};

}
