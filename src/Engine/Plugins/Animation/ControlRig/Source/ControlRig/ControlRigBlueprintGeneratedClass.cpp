#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/ControlRigBlueprintGeneratedClass.h"

#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/ControlRigObjectVersion.h"

namespace ue5parse::runtime {

void UControlRigBlueprintGeneratedClass::Serialize(FArchive& Ar) {
    UBlueprintGeneratedClass::Serialize(Ar);
    if (!Ar.CustomVerAtLeast(FControlRigObjectVersion::GUID, FControlRigObjectVersion::SwitchedToRigVM, EGame::GAME_UE4_25))
        return;
    SerializeVM(Ar);
    if (!Ar.CustomVerAtLeast(FControlRigObjectVersion::GUID, FControlRigObjectVersion::StoreFunctionsInGeneratedClass, EGame::GAME_UE5_2))
        return;
    Ar << GraphFunctionStore;
}

}
