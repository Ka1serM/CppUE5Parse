#pragma once

#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMByteCode.h"
#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMMemoryDeprecated.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "RigVM.generated.h"

namespace ue5parse::runtime {

enum class ERigVMParameterType : uint8 {
    Input,
    Output,
    Invalid,
};

struct FRigVMParameter {
    ERigVMParameterType Type = ERigVMParameterType::Invalid;
    FName Name;
    int32 RegisterIndex = -1;
    FString CPPType;
    FName ScriptStructPath;

    bool Serialize(FArchive& Ar);
    friend FArchive& operator<<(FArchive& Ar, FRigVMParameter& Parameter) {
        Parameter.Serialize(Ar);
        return Ar;
    }
};

UCLASS()
class URigVM : public UObject {
    GENERATED_BODY()
public:
    FRigVMMemoryContainer WorkMemoryStorage;
    FRigVMMemoryContainer LiteralMemoryStorage;
    TArray<FName> FunctionNamesStorage;
    FRigVMByteCode ByteCodeStorage;
    TArray<FRigVMParameter> Parameters;

    void Serialize(FArchive& Ar) override;
};

}
