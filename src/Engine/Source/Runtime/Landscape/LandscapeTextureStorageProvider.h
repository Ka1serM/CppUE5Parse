#pragma once

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
    Result<std::vector<std::byte>> ReadMip(int32 MipIndex) const;
};

}
