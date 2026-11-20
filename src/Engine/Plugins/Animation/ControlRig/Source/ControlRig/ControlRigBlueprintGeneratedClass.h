#pragma once

#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/ControlRigDefines.h"
#include "Engine/Source/Runtime/Engine/Engine/BlueprintGeneratedClass.h"

#include "ControlRigBlueprintGeneratedClass.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UControlRigBlueprintGeneratedClass : public UBlueprintGeneratedClass {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FControlRigOperator> Operators;
};

}
