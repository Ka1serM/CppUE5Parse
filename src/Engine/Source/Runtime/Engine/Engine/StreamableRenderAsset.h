#pragma once

#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"

#include "StreamableRenderAsset.generated.h"

namespace ue5parse::runtime {

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
