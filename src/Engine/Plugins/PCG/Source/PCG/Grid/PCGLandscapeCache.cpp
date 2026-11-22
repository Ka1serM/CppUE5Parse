#include "Engine/Plugins/PCG/Source/PCG/Grid/PCGLandscapeCache.h"

namespace ue5parse::runtime {

void FPCGLandscapeCacheEntry::Serialize(FArchive& Ar) {
    if (Ar.GetGame() < EGame::GAME_UE5_4)
        Ar << Component;
    Ar << PointHalfSize << Stride << LayerDataNames;
    BulkData.Serialize(Ar);
}

void UPCGLandscapeCache::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    const int32 NumEntries = Ar.ReadCount(sizeof(FGuid) + 2 * sizeof(int32));
    CachedKeys.SetNum(NumEntries);
    CachedData.SetNum(NumEntries);
    for (int32 EntryIndex = 0; EntryIndex < NumEntries && !Ar.IsError(); ++EntryIndex) {
        Ar << CachedKeys[EntryIndex].Guid << CachedKeys[EntryIndex].Coordinates.X << CachedKeys[EntryIndex].Coordinates.Y;
        CachedData[EntryIndex].Serialize(Ar);
    }
}

}
