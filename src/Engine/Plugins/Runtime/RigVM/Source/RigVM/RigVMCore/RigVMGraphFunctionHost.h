#pragma once

#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMFunctionCompilationData.h"
#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMGraphFunctionHeader.h"

namespace ue5parse::runtime {

struct FRigVMObjectArchive {
    TMap<FGuid, int32> Versions;
    TArray<uint8> Buffer;
    int32 UncompressedSize = 0;
    int32 CompressedSize = 0;
    bool bIsCompressed = false;
};

struct FRigVMGraphFunctionData {
    FRigVMGraphFunctionHeader Header;
    FRigVMFunctionCompilationData CompilationData;
    FString SerializedCollapsedNode;
    FRigVMObjectArchive CollapseNodeArchive;

    friend FArchive& operator<<(FArchive& Ar, FRigVMGraphFunctionData& Function);
};

struct FRigVMGraphFunctionStore {
    TArray<FRigVMGraphFunctionData> PublicFunctions;

    friend FArchive& operator<<(FArchive& Ar, FRigVMGraphFunctionStore& Store) { return Ar << Store.PublicFunctions; }
};

}
