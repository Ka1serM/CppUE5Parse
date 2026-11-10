#include "Engine/Source/Runtime/Engine/Engine/Level.h"

#include "Engine/Source/Runtime/Engine/Engine/World.h"

namespace ue5parse::runtime {

void ULevel::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.AtEnd())
        return;
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
    UObject* NavListStart = nullptr;
    UObject* NavListEnd = nullptr;
    Ar << NavListStart << NavListEnd;
    Ar.SkipBytes(16 + 4 + 4 + 4 + 4);
    const int32 BucketCount = Ar.ReadCount(12);
    for (int32 i = 0; i < BucketCount && !Ar.IsError(); i++) {
        Ar.SkipBytes(4);
        Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(28)) * 28);
        const int32 ChunkCount = Ar.ReadCount(12);
        for (int32 c = 0; c < ChunkCount && !Ar.IsError(); c++) {
            Ar.SkipBytes(4 + 4);
            Ar.SkipBytes(Ar.ReadCount(1));
        }
    }
    Ar.SkipBytes(4 + 24 + 24 + 1 + 4 * 3);
    Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(4)) * 4);
}

void ULevel::PostLoad() {
    Super::PostLoad();
    if (OwningWorld == nullptr)
        OwningWorld = Cast<UWorld>(GetOuter());
}

}
