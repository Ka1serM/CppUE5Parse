#pragma once

#include "Engine/Plugins/Animation/IKRig/Source/IKRig/Retargeter/IKRetargetOps.h"
#include "Engine/Source/Runtime/Core/Math/Rotator.h"
#include "Engine/Source/Runtime/Core/Math/Vector.h"
#include "Engine/Source/Runtime/Engine/Animation/BoneReference.h"

#include "PelvisMotionOp.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FIKRetargetPelvisMotionOpSettings : public FIKRetargetOpSettingsBase {
    GENERATED_BODY()
    UPROPERTY()
    FBoneReference SourcePelvisBone;
    UPROPERTY()
    FBoneReference TargetPelvisBone;
    UPROPERTY()
    double FloorConstraintWeight = 0.0;
    UPROPERTY()
    double SourceCrotchOffset = 0.0;
    UPROPERTY()
    double TargetCrotchOffset = 0.0;
    UPROPERTY()
    double RotationAlpha = 1.0;
    UPROPERTY()
    FRotator RotationOffsetLocal = FRotator::ZeroRotator;
    UPROPERTY()
    FRotator RotationOffsetGlobal = FRotator::ZeroRotator;
    UPROPERTY()
    double TranslationAlpha = 1.0;
    UPROPERTY()
    FVector TranslationOffsetLocal = FVector::ZeroVector;
    UPROPERTY()
    FVector TranslationOffsetGlobal = FVector::ZeroVector;
    UPROPERTY()
    double BlendToSourceTranslation = 0.0;
    UPROPERTY()
    FVector BlendToSourceTranslationWeights = FVector::OneVector;
    UPROPERTY()
    double ScaleHorizontal = 1.0;
    UPROPERTY()
    double ScaleVertical = 1.0;
    UPROPERTY()
    double AffectIKHorizontal = 1.0;
    UPROPERTY()
    double AffectIKVertical = 0.0;
    UPROPERTY()
    double DebugDrawSize = 5.0;
    UPROPERTY()
    double DebugDrawThickness = 5.0;
    UPROPERTY()
    FVector TranslationOffset_DEPRECATED = FVector::ZeroVector;
    UPROPERTY()
    FRotator RotationOffset_DEPRECATED = FRotator::ZeroRotator;
    UPROPERTY()
    bool bEnableDebugDraw_DEPRECATED = true;
};

USTRUCT()
struct FIKRetargetPelvisMotionOp : public FIKRetargetOpBase {
    GENERATED_BODY()
    UPROPERTY()
    FIKRetargetPelvisMotionOpSettings Settings;
};

}
