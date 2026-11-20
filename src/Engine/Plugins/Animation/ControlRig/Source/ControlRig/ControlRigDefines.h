#pragma once

#include "Engine/Source/Runtime/PropertyPath/PropertyPathHelpers.h"

#include "ControlRigDefines.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EControlRigOpCode : uint8 { Done, Copy, Exec, Invalid };

//! UE 4.23 and 4.24 run a control rig as a list of operators.
USTRUCT()
struct FControlRigOperator {
    GENERATED_BODY()
    UPROPERTY()
    EControlRigOpCode OpCode = EControlRigOpCode::Invalid;
    UPROPERTY()
    FString PropertyPath1_DEPRECATED;
    UPROPERTY()
    FString PropertyPath2_DEPRECATED;
    UPROPERTY()
    FCachedPropertyPath CachedPropertyPath1;
    UPROPERTY()
    FCachedPropertyPath CachedPropertyPath2;
};

}
