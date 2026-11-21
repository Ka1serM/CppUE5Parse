#pragma once

#include <array>
#include <optional>

#include "Engine/Source/Runtime/Engine/Engine/AssetUserData.h"

#include "LandscapeEdgeFixup.generated.h"

namespace ue5parse::runtime {

struct FHeightmapTextureEdgeSnapshot {
    int32 EdgeLength = 0;
    TArray<uint32> EdgeData;
    std::optional<std::array<uint32, 4>> CornerData;
    std::array<uint32, 8> SnapshotEdgeHashes{};
    std::optional<std::array<uint32, 8>> InitialEdgeHashes;
};

UCLASS()
class ULandscapeHeightmapTextureEdgeFixup : public UAssetUserData {
    GENERATED_BODY()
public:
    FHeightmapTextureEdgeSnapshot EdgeSnapshot;

    void Serialize(FArchive& Ar) override;
};

}
