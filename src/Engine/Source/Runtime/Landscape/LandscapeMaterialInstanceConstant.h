#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialInstanceConstant.h"

#include "LandscapeMaterialInstanceConstant.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FLandscapeMaterialTextureStreamingInfo {
    GENERATED_BODY()
    UPROPERTY()
    FName TextureName;
    UPROPERTY()
    float TexelFactor = 1.0f;
};

UCLASS()
class ULandscapeMaterialInstanceConstant : public UMaterialInstanceConstant {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FLandscapeMaterialTextureStreamingInfo> TextureStreamingInfo;
    UPROPERTY()
    int32 DataWeightmapIndex = 0;
    UPROPERTY()
    int32 DataWeightmapSize = 0;
    UPROPERTY()
    uint8 bIsLayerThumbnail : 1 = false;
    UPROPERTY()
    uint8 bDisableTessellation : 1 = false;
    UPROPERTY()
    uint8 bMobile : 1 = false;
    UPROPERTY()
    uint8 bEditorToolUsage : 1 = false;
};

}
