#include "Engine/Source/Runtime/Engine/WorldPartition/WorldPartition.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

void UWorldPartition::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    if (!Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::WorldPartitionSerializeStreamingPolicyOnCook, EGame::GAME_UE5_1))
        return;
    bool bCooked = false;
    Ar << bCooked;
    if (bCooked)
        Ar << StreamingPolicy;
}

}
