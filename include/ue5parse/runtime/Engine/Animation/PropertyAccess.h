#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/FieldPath.h"
#include "ue5parse/runtime/CoreUObject/UObject/Object.h"

#include "PropertyAccess.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FPropertyAccessSegment {
    GENERATED_BODY()
    UPROPERTY()
    FName Name;
    UPROPERTY()
    UStruct* Struct = nullptr;
    UPROPERTY()
    FFieldPath Property;
    UPROPERTY()
    UFunction* Function = nullptr;
    UPROPERTY()
    int32 ArrayIndex = INDEX_NONE;
    UPROPERTY()
    uint16 Flags = 0;
};

USTRUCT()
struct FPropertyAccessPath {
    GENERATED_BODY()
    UPROPERTY()
    int32 PathSegmentStartIndex = 0;
    UPROPERTY()
    int32 PathSegmentCount = 0;
};

USTRUCT()
struct FPropertyAccessCopy {
    GENERATED_BODY()
    UPROPERTY()
    int32 AccessIndex = INDEX_NONE;
    UPROPERTY()
    int32 DestAccessStartIndex = 0;
    UPROPERTY()
    int32 DestAccessEndIndex = 0;
    UPROPERTY()
    uint8 Type = 0;
};

USTRUCT()
struct FPropertyAccessCopyBatch {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FPropertyAccessCopy> Copies;
};

USTRUCT()
struct FPropertyAccessLibrary {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FPropertyAccessSegment> PathSegments;
    UPROPERTY()
    TArray<FPropertyAccessPath> SrcPaths;
    UPROPERTY()
    TArray<FPropertyAccessPath> DestPaths;
    UPROPERTY()
    TArray<FPropertyAccessCopyBatch> CopyBatchArray;

    bool ResolvePath(UObject* Object, const FPropertyAccessPath& Path, const FProperty*& Property, void*& Address) const;
    bool ProcessCopy(UObject* Object, int32 CopyIndex, bool bNegate = false) const;
};

}
