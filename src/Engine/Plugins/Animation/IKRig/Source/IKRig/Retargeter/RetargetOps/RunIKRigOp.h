#pragma once

#include "Engine/Plugins/Animation/IKRig/Source/IKRig/Retargeter/IKRetargetChainMapping.h"
#include "Engine/Plugins/Animation/IKRig/Source/IKRig/Retargeter/IKRetargetOps.h"

#include "RunIKRigOp.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FIKRetargetRunIKRigOpSettings : public FIKRetargetOpSettingsBase {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<const UIKRigDefinition> IKRigAsset = nullptr;
    UPROPERTY()
    TArray<FName> ExcludedGoals;
    UPROPERTY()
    bool bDrawGoals = true;
    UPROPERTY()
    bool bDrawGoalBoneLocations = true;
    UPROPERTY()
    double GoalDrawSize = 5.0;
    UPROPERTY()
    double GoalDrawThickness = 1.0;
};

USTRUCT()
struct FIKRetargetRunIKRigOp : public FIKRetargetOpBase {
    GENERATED_BODY()
    UPROPERTY()
    FIKRetargetRunIKRigOpSettings Settings;
    UPROPERTY()
    FRetargetChainMapping ChainMapping;
};

}
