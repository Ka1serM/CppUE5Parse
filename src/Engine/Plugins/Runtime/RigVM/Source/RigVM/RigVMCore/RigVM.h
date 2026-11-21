#pragma once

#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMByteCode.h"
#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMMemoryDeprecated.h"
#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMMemoryStorageStruct.h"
#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMPropertyPath.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMRegistry.h"

#include <optional>

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
    uint32 CachedVMHash = 0;
    uint32 CachedCanonicalVMHash = 0;
    TArray<FRigVMPropertyPathDescription> ExternalPropertyPathDescriptions;
    TArray<FName> FunctionNamesStorage;
    FRigVMByteCode ByteCodeStorage;
    TArray<FRigVMParameter> Parameters;
    FRigVMMemoryStorageStruct LiteralMemoryStorageStruct;
    FRigVMMemoryStorageStruct DefaultWorkMemoryStorage;
    FRigVMMemoryStorageStruct DefaultDebugMemoryStorage_DEPRECATED;

    std::optional<FRigVMRegistry_NoLock> LocalizedRegistry;

    void Serialize(FArchive& Ar) override;

private:
    //! Returns false when the VM stored only the deprecated memory containers, after which UE resets it.
    bool SerializeUClassBasedStorage(FArchive& Ar);
    void SerializeLocalizedRegistry(FArchive& Ar);
};

}
