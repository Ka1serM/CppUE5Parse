#include "Engine/Source/Runtime/Landscape/LandscapeEdgeFixup.h"

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

struct FHeightmapTextureEdgeSnapshotCustomVersion {
    static constexpr FGuid GUID{0x12345678, 0x12345678, 0x12345678, 0x12345678};
    enum Type : int32 { BeforeInitialHashWasAdded = 1, BeforeCornerDataWasRemoved = 2 };
};

void ULandscapeHeightmapTextureEdgeFixup::Serialize(FArchive& Ar) {
    using Version = FHeightmapTextureEdgeSnapshotCustomVersion;
    Ar << EdgeSnapshot.EdgeLength << EdgeSnapshot.EdgeData;
    if (!Ar.CustomVerAtLeast(Version::GUID, Version::BeforeCornerDataWasRemoved + 1, EGame::GAME_UE5_6))
        for (uint32& Corner : EdgeSnapshot.CornerData.emplace())
            Ar << Corner;
    for (uint32& Hash : EdgeSnapshot.SnapshotEdgeHashes)
        Ar << Hash;
    if (Ar.CustomVerAtLeast(Version::GUID, Version::BeforeInitialHashWasAdded + 1, EGame::GAME_UE5_6))
        for (uint32& Hash : EdgeSnapshot.InitialEdgeHashes.emplace())
            Ar << Hash;
}

}
