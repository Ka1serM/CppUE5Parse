#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVM.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

bool FRigVMParameter::Serialize(FArchive& Ar) {
    if (Ar.CustomVer(FAnimObjectVersion::GUID) < FAnimObjectVersion::StoreMarkerNamesOnSkeleton)
        return false;
    Ar << Type << Name << RegisterIndex << CPPType << ScriptStructPath;
    return true;
}

void URigVM::Serialize(FArchive& Ar) {
    if (Ar.CustomVer(FAnimObjectVersion::GUID) < FAnimObjectVersion::StoreMarkerNamesOnSkeleton)
        return;
    Ar << WorkMemoryStorage << LiteralMemoryStorage << FunctionNamesStorage << ByteCodeStorage << Parameters;
}

}
