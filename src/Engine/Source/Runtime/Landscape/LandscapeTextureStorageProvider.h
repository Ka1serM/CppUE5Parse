#pragma once

#include <span>
#include <vector>

#include "CppUE5Parse/Core/Result.h"
#include "CppUE5Parse/Core/Versions.h"
#include "Engine/Source/Runtime/CoreUObject/Serialization/BulkData.h"
#include "Engine/Source/Runtime/Engine/Engine/TextureAllMipDataProviderFactory.h"

#include "LandscapeTextureStorageProvider.generated.h"

namespace ue5parse::runtime {

class UTexture2D;

USTRUCT()
struct FLandscapeTexture2DMipMap {
    GENERATED_BODY()
    UPROPERTY()
    int32 SizeX = 0;
    UPROPERTY()
    int32 SizeY = 0;
    UPROPERTY()
    bool bCompressed = false;

    FByteBulkData BulkData;

    void Serialize(FArchive& Ar);
};

UCLASS()
class ULandscapeTextureStorageProviderFactory : public UTextureAllMipDataProviderFactory {
    GENERATED_BODY()
public:
    UPROPERTY()
    int32 BoundaryOffset = 0;
    UPROPERTY()
    int32 BoundaryCountX = 0;
    UPROPERTY()
    int32 BoundaryCountY = 0;

    int32 NumNonOptionalMips = 0;
    int32 NumNonStreamingMips = 0;
    FVector LandscapeGridScale = FVector::ZeroVector;
    TArray<FLandscapeTexture2DMipMap> Mips;
    TObjectPtr<UTexture2D> Texture = nullptr;

    void Serialize(FArchive& Ar) override;
    Result<std::vector<std::byte>> GetMipData(int32 MipIndex) const;
    Result<void> DecompressMip(std::span<const std::byte> Source, std::span<std::byte> Dest, int32 MipIndex, EGame Game) const;

private:
    Result<void> DecompressMipOriginalUnoptimized(const uint8* SourceData, int64 SourceDataBytes, uint8* DestData, int32 MipIndex) const;
};

}
