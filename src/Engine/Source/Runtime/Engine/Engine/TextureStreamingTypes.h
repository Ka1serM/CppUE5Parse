#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "TextureStreamingTypes.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FStreamingTextureBuildInfo {
    GENERATED_BODY()
    UPROPERTY()
    uint32 PackedRelativeBox = 0;
    UPROPERTY()
    int32 TextureLevelIndex = 0;
    UPROPERTY()
    float TexelFactor = 0;
};

}
