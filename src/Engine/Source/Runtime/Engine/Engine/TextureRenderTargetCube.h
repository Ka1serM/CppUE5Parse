#pragma once

#include "Engine/Source/Runtime/Engine/Engine/TextureRenderTarget.h"

#include "TextureRenderTargetCube.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UTextureRenderTargetCube : public UTextureRenderTarget {
    GENERATED_BODY()
public:
    UPROPERTY()
    int32 SizeX = 0;
    UPROPERTY()
    FLinearColor ClearColor = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);
    UPROPERTY()
    TEnumAsByte<EPixelFormat> OverrideFormat = EPixelFormat::PF_Unknown;
    UPROPERTY()
    uint8 bHDR : 1 = 1;
    UPROPERTY()
    uint8 bSupportsUAV : 1 = 0;
    UPROPERTY()
    uint8 bForceLinearGamma : 1 = 1;
    UPROPERTY()
    uint8 bAutoGenerateMips : 1 = 0;
    UPROPERTY()
    TEnumAsByte<TextureFilter> MipsSamplerFilter = TF_Default;
};

}
