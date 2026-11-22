#include "Engine/Plugins/Runtime/StateTree/Source/StateTreeModule/StateTreeInstanceData.h"

namespace ue5parse::runtime {

//! Before UE 5.4 the struct has no native serializer, and UE 5.4 reads an older save as tagged properties.
bool FStateTreeInstanceData::Serialize(FArchive& Ar) {
    if (!Ar.CustomVerAtLeast(FStateTreeInstanceStorageCustomVersion::GUID, FStateTreeInstanceStorageCustomVersion::AddedCustomSerialization, EGame::GAME_UE5_4))
        return false;
    UScriptStruct* Storage = FStateTreeInstanceStorage::StaticStruct();
    InstanceStorage.InitializeAs(Storage);
    Storage->SerializeItem(Ar, InstanceStorage.GetMutableMemory());
    return true;
}

}
