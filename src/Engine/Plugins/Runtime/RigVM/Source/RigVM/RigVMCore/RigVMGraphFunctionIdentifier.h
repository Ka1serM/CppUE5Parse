#pragma once

#include "Engine/Source/Runtime/Core/Containers/UnrealString.h"
#include "Engine/Source/Runtime/Core/Templates/TypeHash.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"

#include "RigVMGraphFunctionIdentifier.generated.h"

namespace ue5parse::runtime {

//! UE before 5.5 stores the library node as a soft object path, which the port keeps as its string.
USTRUCT()
struct FRigVMGraphFunctionIdentifier {
    GENERATED_BODY()
    UPROPERTY()
    FString LibraryNodePath;
    UPROPERTY()
    FSoftObjectPath HostObject;

    bool operator==(const FRigVMGraphFunctionIdentifier& Other) const = default;
    friend uint32 GetTypeHash(const FRigVMGraphFunctionIdentifier& Pointer) {
        return HashCombine(GetTypeHash(Pointer.LibraryNodePath), GetTypeHash(Pointer.HostObject));
    }
    friend FArchive& operator<<(FArchive& Ar, FRigVMGraphFunctionIdentifier& Data);
};

}
