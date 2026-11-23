#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/Core/UObject/NameTypes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"

#include "RigVMExternalVariable.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FRigVMExternalVariableDef {
    GENERATED_BODY()
    UPROPERTY()
    FGuid Guid;
    UPROPERTY()
    FName Name;
    UPROPERTY()
    FName BaseCPPType;
    //! UE resolves the stored path to the type object; the port keeps the path.
    UPROPERTY()
    FSoftObjectPath CPPTypeObject;
    UPROPERTY()
    bool bIsArray = false;
    UPROPERTY()
    bool bIsPublic = false;
    UPROPERTY()
    bool bIsReadOnly = false;

    friend FArchive& operator<<(FArchive& Ar, FRigVMExternalVariableDef& Variable);
};

USTRUCT()
struct FRigVMExternalVariable : public FRigVMExternalVariableDef {
    GENERATED_BODY()

    friend FArchive& operator<<(FArchive& Ar, FRigVMExternalVariable& Variable) { return Ar << static_cast<FRigVMExternalVariableDef&>(Variable); }
};

}
