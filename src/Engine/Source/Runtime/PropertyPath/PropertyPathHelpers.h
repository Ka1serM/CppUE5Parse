#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "PropertyPathHelpers.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FPropertyPathSegment {
    GENERATED_BODY()
    UPROPERTY()
    FName Name;
    UPROPERTY()
    int32 ArrayIndex = INDEX_NONE;
};

USTRUCT()
struct FCachedPropertyPath {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FPropertyPathSegment> Segments;
    UPROPERTY()
    TObjectPtr<UFunction> CachedFunction = nullptr;
};

}
