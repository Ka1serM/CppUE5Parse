#pragma once

#include "Engine/Source/Runtime/Core/Containers/Array.h"
#include "Engine/Source/Runtime/Core/Containers/UnrealString.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

namespace ue5parse::runtime {

class UScriptStruct;

enum class ERigVMPinDirection : uint8 {
    Input,
    Output,
    IO,
    Visible,
    Hidden,
    Invalid,
};

enum class ERigVMFunctionArgumentDirection : uint8 {
    Input,
    Output,
    Invalid,
};

struct FRigVMFunctionArgument {
    FString Name;
    FString Type;
    ERigVMFunctionArgumentDirection Direction = ERigVMFunctionArgumentDirection::Invalid;

    void Serialize_NoLock(FArchive& Ar);
};

struct FRigVMFunction {
    FString Name;
    TObjectPtr<UScriptStruct> Struct;
    //! UE resolves this struct to the registry's dispatch factory; the port keeps the struct.
    TObjectPtr<UScriptStruct> FactoryStruct;
    TArray<FRigVMFunctionArgument> Arguments;

    void Serialize_NoLock(FArchive& Ar);
};

}
