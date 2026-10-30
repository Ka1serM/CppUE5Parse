#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/Object.h"

#include "BoneMaskFilter.generated.h"

namespace ue5parse::runtime {

// A branch of the skeleton that a layer blends: the bone and its children, the weight rising to 1 over BlendDepth
// generations (UE's Animation/AnimData/BoneMaskFilter.h).
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
