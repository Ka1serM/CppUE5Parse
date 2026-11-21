#include "Engine/Plugins/PCG/Source/PCG/Grid/PCGLandscapeCache.h"

namespace ue5parse::runtime {

void FPCGLandscapeCacheEntry::Serialize(FArchive& Ar) {
    Ar << Component << PointHalfSize << Stride << LayerDataNames;
    BulkData.Serialize(Ar);
}

void UPCGLandscapeCache::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    int32 NumEntries = 0;
    Ar << NumEntries;
    CachedKeys.SetNum(NumEntries);
    CachedData.SetNum(NumEntries);
    for (int32 EntryIndex = 0; EntryIndex < NumEntries; ++EntryIndex) {
        Ar << CachedKeys[EntryIndex].Guid << CachedKeys[EntryIndex].Coordinates.X << CachedKeys[EntryIndex].Coordinates.Y;
        CachedData[EntryIndex].Serialize(Ar);
    }
}

}
