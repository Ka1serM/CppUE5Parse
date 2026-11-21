#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMGraphFunctionHost.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

FArchive& operator<<(FArchive& Ar, FRigVMGraphFunctionData& Function) {
    Ar << Function.Header << Function.CompilationData;
    if (!Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::RigVMSaveSerializedGraphInGraphFunctionData, EGame::GAME_UE5_2))
        return Ar;
    Ar << Function.SerializedCollapsedNode;
    if (!FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::RigVMSaveSerializedGraphInGraphFunctionDataAsByteArray))
        return Ar;
    FRigVMObjectArchive& Archive = Function.CollapseNodeArchive;
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::ObjectArchiveVersionContainerSerialization))
        Ar << Archive.Versions;
    return Ar << Archive.Buffer << Archive.UncompressedSize << Archive.CompressedSize << Archive.bIsCompressed;
}

}
