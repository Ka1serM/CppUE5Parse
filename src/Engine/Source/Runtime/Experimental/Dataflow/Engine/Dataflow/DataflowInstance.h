#pragma once

#include "Engine/Source/Runtime/CoreUObject/StructUtils/PropertyBag.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"

#include "DataflowInstance.generated.h"

namespace ue5parse::runtime {

class UDataflow;

USTRUCT()
struct FDataflowVariableOverrides {
    GENERATED_BODY()
    UPROPERTY()
    FInstancedPropertyBag Variables;
    UPROPERTY()
    TArray<FGuid> OverriddenVariableGuids;
};

USTRUCT()
struct FDataflowInstance {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UDataflow> DataflowAsset = nullptr;
    UPROPERTY()
    FName DataflowTerminal;
    UPROPERTY()
    FDataflowVariableOverrides VariableOverrides;
    UPROPERTY()
    TObjectPtr<UObject> Owner = nullptr;
};

}
