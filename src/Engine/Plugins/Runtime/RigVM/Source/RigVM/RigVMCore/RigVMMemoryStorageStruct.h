#pragma once

#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMMemoryCommon.h"
#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMPropertyPath.h"
#include "Engine/Source/Runtime/CoreUObject/StructUtils/PropertyBag.h"

namespace ue5parse::runtime {

struct FRigVMMemoryStorageStruct : FInstancedPropertyBag {
    ERigVMMemoryType MemoryType = ERigVMMemoryType::Invalid;
    TArray<FRigVMPropertyPathDescription> PropertyPathDescriptions;

    bool Serialize(FArchive& Ar) {
        FInstancedPropertyBag::Serialize(Ar);
        Ar << MemoryType << PropertyPathDescriptions;
        return true;
    }
    friend FArchive& operator<<(FArchive& Ar, FRigVMMemoryStorageStruct& Storage) {
        Storage.Serialize(Ar);
        return Ar;
    }
};

}
