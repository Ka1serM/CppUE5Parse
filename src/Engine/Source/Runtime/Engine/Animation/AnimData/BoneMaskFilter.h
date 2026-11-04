#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "BoneMaskFilter.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FBranchFilter {
    GENERATED_BODY()
    UPROPERTY()
    FName BoneName;
    UPROPERTY()
    int32 BlendDepth = 0;
};

USTRUCT()
struct FInputBlendPose {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FBranchFilter> BranchFilters;
};

}
