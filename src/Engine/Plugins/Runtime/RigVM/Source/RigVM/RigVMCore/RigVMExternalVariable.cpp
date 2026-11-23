#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMExternalVariable.h"

#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMObjectVersion.h"

namespace ue5parse::runtime {

//! UE 5.8 no longer keeps the size that follows the flags.
FArchive& operator<<(FArchive& Ar, FRigVMExternalVariableDef& Variable) {
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::GuidForVariables))
        Ar << Variable.Guid;
    Ar << Variable.Name << Variable.BaseCPPType << Variable.CPPTypeObject << Variable.bIsArray << Variable.bIsPublic << Variable.bIsReadOnly;
    int32 DummySize = 0;
    return Ar << DummySize;
}

}
