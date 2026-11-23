#pragma once

#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMGraphFunctionDefinition.h"

#include "RigVMGraphFunctionHost.generated.h"

namespace ue5parse::runtime {

//! UE also serializes the private functions, but only for object reference collectors.
USTRUCT()
struct FRigVMGraphFunctionStore {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FRigVMGraphFunctionData> PublicFunctions;

    friend FArchive& operator<<(FArchive& Ar, FRigVMGraphFunctionStore& Host) { return Ar << Host.PublicFunctions; }
};

}
