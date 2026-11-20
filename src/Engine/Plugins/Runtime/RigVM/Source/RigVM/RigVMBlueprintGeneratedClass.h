#pragma once

#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMVariant.h"
#include "Engine/Source/Runtime/Engine/Engine/BlueprintGeneratedClass.h"

#include "RigVMBlueprintGeneratedClass.generated.h"

namespace ue5parse::runtime {

UCLASS()
class URigVMBlueprintGeneratedClass : public UBlueprintGeneratedClass {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FName> SupportedEventNames;
    UPROPERTY()
    FRigVMVariant AssetVariant;
};

}
