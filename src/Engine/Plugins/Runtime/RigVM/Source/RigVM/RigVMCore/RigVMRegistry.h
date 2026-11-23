#pragma once

#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMDispatchFactory.h"

namespace ue5parse::runtime {

//! The port reads only the localized registry that a VM stores, not the global registry of the native types.
struct FRigVMRegistry_NoLock {
    struct FTypeInfo {
        FRigVMTemplateArgumentType Type;

        void Serialize_NoLock(FArchive& Ar) { Type.Serialize_NoLock(Ar); }
    };

    //! UE keeps a TSet; the port keeps the serialized order and does not resolve the classes.
    TArray<TObjectPtr<const UClass>> AllowedClasses;
    TArray<FTypeInfo> Types;
    TArray<FRigVMDispatchFactory> Factories;
    TArray<FRigVMFunction> Functions;

    void Serialize_NoLock(FArchive& Ar);
};

}
