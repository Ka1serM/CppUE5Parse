#pragma once

#include "ue5parse/runtime/Core/Containers/EnumAsByte.h"
#include "ue5parse/runtime/CoreUObject/UObject/Object.h"

#include "ExposedValueHandler.generated.h"

namespace ue5parse::runtime {

struct FPropertyAccessLibrary;

namespace EPostCopyOperation {
enum Type : uint8 { None, LogicalNegateBool };
}

USTRUCT()
struct FExposedValueCopyRecord {
    GENERATED_BODY()
    UPROPERTY()
    int32 CopyIndex = INDEX_NONE;
    UPROPERTY()
    TEnumAsByte<EPostCopyOperation::Type> PostCopyOperation = EPostCopyOperation::None;
};

USTRUCT()
struct FExposedValueHandler {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FExposedValueCopyRecord> CopyRecords;
    UPROPERTY()
    TObjectPtr<UFunction> Function = nullptr;
    UPROPERTY()
    FName BoundFunction;

    // Runs the bound function of the instance, then the copies into the node's inputs.
    void Execute(const struct FAnimationBaseContext& Context) const;
};

}
