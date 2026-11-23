#pragma once

#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMFunction.h"
#include "Engine/Source/Runtime/Core/UObject/NameTypes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "RigVMTemplate.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FRigVMTemplateArgumentType {
    GENERATED_BODY()
    UPROPERTY()
    FName CPPType;
    UPROPERTY()
    TObjectPtr<UObject> CPPTypeObject;

    void Serialize_NoLock(FArchive& Ar) { Ar << CPPType << CPPTypeObject; }
};

struct FRigVMTemplateArgumentInfo {
    FName Name;
    ERigVMPinDirection Direction = ERigVMPinDirection::Invalid;
    //! UE resolves each type to its index in the registry; the port keeps the serialized types.
    TArray<FRigVMTemplateArgumentType> Types;
};

}
