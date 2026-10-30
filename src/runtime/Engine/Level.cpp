#include "ue5parse/runtime/Engine/Engine/Level.h"

// UE's Level.cpp (UE 5.3).

namespace ue5parse::runtime {

void ULevel::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.AtEnd()) return;
    const int32 ActorCount = Ar.ReadCount(4);
    Actors.Empty(ActorCount);
    for (int32 i = 0; i < ActorCount && !Ar.IsError(); i++) {
        UObject* Actor = nullptr;
        Ar << Actor;
        Actors.Add(Cast<AActor>(Actor));
    }
    Ar << URL;
    Ar << Model;
    Ar << ModelComponents;
    Ar << LevelScriptActor;
    // NavListStart and NavListEnd: the navigation objects of the level. Nothing reads them.
    UObject* NavListStart = nullptr;
    UObject* NavListEnd = nullptr;
    Ar << NavListStart << NavListEnd;
    // FPrecomputedVisibilityHandler: bucket origin (FVector2D), cell sizes, bucket size, bucket count, then the buckets.
    Ar.SkipBytes(16 + 4 + 4 + 4 + 4);
    const int32 BucketCount = Ar.ReadCount(12);
    for (int32 i = 0; i < BucketCount && !Ar.IsError(); i++) {
        Ar.SkipBytes(4);                                                // CellDataSize
        Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(28)) * 28);        // cells: FVector, chunk index, data offset
        const int32 ChunkCount = Ar.ReadCount(12);
        for (int32 c = 0; c < ChunkCount && !Ar.IsError(); c++) {
            Ar.SkipBytes(4 + 4);                                        // bCompressed, UncompressedSize
            Ar.SkipBytes(Ar.ReadCount(1));                              // Data
        }
    }
    // FPrecomputedVolumeDistanceField: VolumeMaxDistance, VolumeBox, sizes, then the data.
    Ar.SkipBytes(4 + 24 + 24 + 1 + 4 * 3);
    Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(4)) * 4);
}

}
