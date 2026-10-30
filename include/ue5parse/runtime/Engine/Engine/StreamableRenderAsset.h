#pragma once

#include "ue5parse/runtime/Engine/Engine/EngineTypes.h"

#include "StreamableRenderAsset.generated.h"

namespace ue5parse::runtime {

// UStreamableRenderAsset: the base of textures and meshes (UE's Engine/StreamableRenderAsset.h). It has no cooked data
// of its own.
UCLASS(Abstract)
class UStreamableRenderAsset : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    int32 NumCinematicMipLevels = 0;
    UPROPERTY()
    bool NeverStream = false;
};

}
