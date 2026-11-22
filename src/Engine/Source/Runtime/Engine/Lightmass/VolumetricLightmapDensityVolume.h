#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Volume.h"

#include "VolumetricLightmapDensityVolume.generated.h"

namespace ue5parse::runtime {

UCLASS()
class AVolumetricLightmapDensityVolume : public AVolume {
    GENERATED_BODY()
public:
    explicit AVolumetricLightmapDensityVolume(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
