#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

//! UE also serializes the private functions, but only for object reference collectors.
struct FRigVMGraphFunctionStore {
    int32 NumPublicFunctions = 0;

    //! The port does not read FRigVMGraphFunctionData yet, so a store with public functions is an error.
    friend FArchive& operator<<(FArchive& Ar, FRigVMGraphFunctionStore& Store) {
        Ar << Store.NumPublicFunctions;
        if (Store.NumPublicFunctions != 0)
            Ar.SetError(ErrorCode::InvalidData, "FRigVMGraphFunctionStore.PublicFunctions");
        return Ar;
    }
};

}
