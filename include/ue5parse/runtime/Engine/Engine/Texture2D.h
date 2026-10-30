#pragma once

#include "ue5parse/runtime/Engine/Engine/Texture.h"

#include "Texture2D.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UTexture2D : public UTexture {
    GENERATED_BODY()
public:
    UPROPERTY()
    TEnumAsByte<TextureAddress> AddressX = TA_Wrap;
    UPROPERTY()
    TEnumAsByte<TextureAddress> AddressY = TA_Wrap;
    UPROPERTY()
    FIntPoint ImportedSize;

    void Serialize(FArchive& Ar) override;
    UFUNCTION(BlueprintCallable)
    int32 Blueprint_GetSizeX() const { return GetSizeX(); }
    UFUNCTION(BlueprintCallable)
    int32 Blueprint_GetSizeY() const { return GetSizeY(); }
    int32 GetSizeX() const { return PlatformData.SizeX; }
    int32 GetSizeY() const { return PlatformData.SizeY; }
    int32 GetNumMips() const { return PlatformData.Mips.Num(); }
};

}
