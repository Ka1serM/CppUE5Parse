#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Volume.h"
#include "Engine/Source/Runtime/Engine/Engine/Scene.h"

#include "PostProcessVolume.generated.h"

namespace ue5parse::runtime {

UCLASS()
class APostProcessVolume : public AVolume {
    GENERATED_BODY()
public:
    UPROPERTY()
    FPostProcessSettings Settings;
    UPROPERTY()
    float Priority = 0.0f;
    UPROPERTY()
    float BlendRadius = 100.0f;
    UPROPERTY()
    float BlendWeight = 1.0f;
    UPROPERTY()
    uint8 bEnabled : 1 = true;
    UPROPERTY()
    uint8 bUnbound : 1 = false;
    UPROPERTY()
    FGuid VolumeGuid;

    explicit APostProcessVolume(const FObjectInitializer& ObjectInitializer);
};

}
