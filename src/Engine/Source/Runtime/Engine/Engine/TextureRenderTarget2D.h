#pragma once

#include "Engine/Source/Runtime/Engine/Engine/TextureRenderTarget.h"

#include "TextureRenderTarget2D.generated.h"

namespace ue5parse::runtime {

UENUM()
enum ETextureRenderTargetFormat : int {
    RTF_R8,
    RTF_RG8,
    RTF_RGBA8,
    RTF_RGBA8_SRGB,
    RTF_R16f,
    RTF_RG16f,
    RTF_RGBA16f,
    RTF_R32f,
    RTF_RG32f,
    RTF_RGBA32f,
    RTF_RGB10A2,
};

UCLASS()
class UTextureRenderTarget2D : public UTextureRenderTarget {
    GENERATED_BODY()
public:
    UPROPERTY()
    int32 SizeX = 1;
    UPROPERTY()
    int32 SizeY = 1;
    UPROPERTY()
    FLinearColor ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
    UPROPERTY()
    TEnumAsByte<TextureAddress> AddressX = TA_Wrap;
    UPROPERTY()
    TEnumAsByte<TextureAddress> AddressY = TA_Wrap;
    UPROPERTY()
    uint8 bForceLinearGamma : 1 = 1;
    UPROPERTY()
    uint8 bHDR_DEPRECATED : 1 = 1;
    UPROPERTY()
    uint8 bGPUSharedFlag : 1 = 0;
    UPROPERTY()
    TEnumAsByte<ETextureRenderTargetFormat> RenderTargetFormat = RTF_RGBA16f;
    UPROPERTY()
    uint8 bSupportsUAV : 1 = 0;
    UPROPERTY()
    uint8 bAutoGenerateMips : 1 = 0;
    UPROPERTY()
    TEnumAsByte<TextureFilter> MipsSamplerFilter = TF_Default;
    UPROPERTY()
    TEnumAsByte<TextureAddress> MipsAddressU = TA_Clamp;
    UPROPERTY()
    TEnumAsByte<TextureAddress> MipsAddressV = TA_Clamp;
    UPROPERTY()
    TEnumAsByte<EPixelFormat> OverrideFormat = EPixelFormat::PF_Unknown;
};

}
