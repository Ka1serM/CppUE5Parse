#include "Engine/Plugins/Runtime/StateTree/Source/StateTreeModule/StateTreeInstanceData.h"

namespace ue5parse::runtime {

bool FStateTreeInstanceData::Serialize(FArchive& Ar) {
    if (Ar.GetGame() < EGame::GAME_UE5_4)
        return false;
    constexpr FGuid StorageVersion{0x60C4F0DE, 0x8B264C34, 0xAA937201, 0x5DFF09CC};
    UScriptStruct* Storage = FStateTreeInstanceStorage::StaticStruct();
    InstanceStorage.InitializeAs(Storage);
    if (Ar.CustomVerAtLeast(StorageVersion, 1, EGame::GAME_UE5_4))
        Storage->SerializeItem(Ar, InstanceStorage.GetMutableMemory());
    return true;
}

}
