#pragma once

#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMGraphFunctionHost.h"
#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMVariant.h"
#include "Engine/Source/Runtime/Engine/Engine/BlueprintGeneratedClass.h"

#include "RigVMBlueprintGeneratedClass.generated.h"

namespace ue5parse::runtime {

class URigVM;

UCLASS()
class URigVMBlueprintGeneratedClass : public UBlueprintGeneratedClass {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FName> SupportedEventNames;
    UPROPERTY()
    FRigVMVariant AssetVariant;
    //! UE copies the loaded VM into the VM of the class default object; the port keeps it on the class.
    URigVM* VM = nullptr;
    FRigVMGraphFunctionStore GraphFunctionStore;

    void Serialize(FArchive& Ar) override;

protected:
    void SerializeVM(FArchive& Ar);
};

}
