#include "Engine/Source/Runtime/Engine/Animation/SmartName.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

//! A cook leaves out the GUID that UE 4.17 and older save with a smart name.
bool FSmartName::Serialize(FArchive& Ar) {
    Ar << DisplayName;
    if (Ar.CustomVerAtLeast(FAnimPhysObjectVersion::GUID, FAnimPhysObjectVersion::RemoveUIDFromSmartNameSerialize, EGame::GAME_UE4_16))
        return true;
    uint16 TempUID = 0;
    Ar << TempUID;
    return true;
}

}
