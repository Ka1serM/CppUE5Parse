#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/ControlRigBlueprintGeneratedClass.h"

#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/ControlRigObjectVersion.h"
#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVM.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"

namespace ue5parse::runtime {

void UControlRigBlueprintGeneratedClass::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.CustomVer(FControlRigObjectVersion::GUID) < FControlRigObjectVersion::SwitchedToRigVM)
        return;
    VM = NewObject<URigVM>(GetTransientPackage());
    VM->Serialize(Ar);
}

}
