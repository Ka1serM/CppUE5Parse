#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMBlueprintGeneratedClass.h"

#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVM.h"
#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"

namespace ue5parse::runtime {

void URigVMBlueprintGeneratedClass::SerializeVM(FArchive& Ar) {
    VM = NewObject<URigVM>(GetTransientPackage());
    VM->Serialize(Ar);
}

void URigVMBlueprintGeneratedClass::Serialize(FArchive& Ar) {
    UBlueprintGeneratedClass::Serialize(Ar);
    if (!Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::RigVMGeneratedClass, EGame::GAME_UE5_2))
        return;
    SerializeVM(Ar);
    Ar << GraphFunctionStore;
}

}
