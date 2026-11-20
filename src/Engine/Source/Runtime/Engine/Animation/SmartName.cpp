#include "Engine/Source/Runtime/Engine/Animation/SmartName.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

bool FSmartName::Serialize(FArchive& Ar) {
    Ar << DisplayName;
    if (!Ar.CustomVerAtLeast(FAnimPhysObjectVersion::GUID, FAnimPhysObjectVersion::RemoveUIDFromSmartNameSerialize, EGame::GAME_UE4_16)) {
        uint16 TempUID = 0;
        Ar << TempUID;
    }
    if (!Ar.CustomVerAtLeast(FAnimPhysObjectVersion::GUID, FAnimPhysObjectVersion::SmartNameRefactorForDeterministicCooking, EGame::GAME_UE4_18)) {
        FGuid TempGUID;
        Ar << TempGUID;
    }
    return true;
}

}
