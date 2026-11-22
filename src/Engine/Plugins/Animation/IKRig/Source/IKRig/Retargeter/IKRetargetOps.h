#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "IKRetargetOps.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FIKRetargetOpSettingsBase {
    GENERATED_BODY()
    UPROPERTY()
    int32 LODThreshold = INDEX_NONE;
    UPROPERTY()
    bool bDebugDraw = true;
};

USTRUCT()
struct FIKRetargetOpBase {
    GENERATED_BODY()
    UPROPERTY()
    bool bIsEnabled = true;
    UPROPERTY()
    FName Name;
    UPROPERTY()
    FName ParentOpName;
};

}
