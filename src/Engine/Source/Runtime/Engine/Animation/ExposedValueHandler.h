#pragma once

#include "Engine/Source/Runtime/Core/Containers/EnumAsByte.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

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

    void Execute(const struct FAnimationBaseContext& Context) const;
};

}
