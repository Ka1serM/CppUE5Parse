#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "PCGDataType.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FPCGDataTypeBaseId {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<const UScriptStruct> Struct = nullptr;
};

}
