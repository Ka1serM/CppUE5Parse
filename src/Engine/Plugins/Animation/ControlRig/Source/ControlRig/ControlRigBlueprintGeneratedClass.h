#pragma once

#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/ControlRigDefines.h"
#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMBlueprintGeneratedClass.h"

#include "ControlRigBlueprintGeneratedClass.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UControlRigBlueprintGeneratedClass : public URigVMBlueprintGeneratedClass {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FControlRigOperator> Operators;

    void Serialize(FArchive& Ar) override;
};

}
