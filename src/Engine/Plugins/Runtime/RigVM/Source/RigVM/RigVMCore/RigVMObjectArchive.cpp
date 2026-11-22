#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMObjectArchive.h"

#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMObjectVersion.h"

namespace ue5parse::runtime {

//! Before ObjectArchiveVersionContainerSerialization UE takes the versions of the archive from the outer archive.
FArchive& operator<<(FArchive& Ar, FRigVMObjectArchive& Data) {
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::ObjectArchiveVersionContainerSerialization))
        Ar << Data.Versions;
    return Ar << Data.Buffer << Data.UncompressedSize << Data.CompressedSize << Data.bIsCompressed;
}

}
