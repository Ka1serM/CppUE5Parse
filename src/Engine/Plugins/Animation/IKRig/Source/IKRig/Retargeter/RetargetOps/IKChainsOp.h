#pragma once

#include "Engine/Plugins/Animation/IKRig/Source/IKRig/Retargeter/IKRetargetOps.h"
#include "Engine/Source/Runtime/Core/Math/Rotator.h"
#include "Engine/Source/Runtime/Core/Math/Vector.h"

#include "IKChainsOp.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FRetargetIKChainSettings {
    GENERATED_BODY()
    UPROPERTY()
    FName TargetChainName;
    UPROPERTY()
    bool EnableIK = true;
    UPROPERTY()
    double BlendToSource = 0.0;
    UPROPERTY()
    double BlendToSourceTranslation = 1.0;
    UPROPERTY()
    double BlendToSourceRotation = 0.0;
    UPROPERTY()
    FVector BlendToSourceWeights = FVector::OneVector;
    UPROPERTY()
    bool ApplyPelvisOffsetToSourceGoals = false;
    UPROPERTY()
    FVector StaticOffset = FVector::ZeroVector;
    UPROPERTY()
    FVector StaticLocalOffset = FVector::ZeroVector;
    UPROPERTY()
    FRotator StaticRotationOffset = FRotator::ZeroRotator;
    UPROPERTY()
    double ScaleVertical = 1.0;
    UPROPERTY()
    double Extension = 1.0;
};

USTRUCT()
struct FIKRetargetIKChainsOpSettings : public FIKRetargetOpSettingsBase {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FRetargetIKChainSettings> ChainsToRetarget;
    UPROPERTY()
    bool bDrawFinalGoals = true;
    UPROPERTY()
    bool bDrawSourceLocations = true;
    UPROPERTY()
    double GoalDrawSize = 5.0;
    UPROPERTY()
    double GoalDrawThickness = 1.0;
};

USTRUCT()
struct FIKRetargetIKChainsOp : public FIKRetargetOpBase {
    GENERATED_BODY()
    UPROPERTY()
    FIKRetargetIKChainsOpSettings Settings;
};

}
