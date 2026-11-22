#pragma once

#include "Engine/Plugins/Animation/IKRig/Source/IKRig/Retargeter/IKRetargetOps.h"
#include "Engine/Source/Runtime/Core/Math/Transform.h"
#include "Engine/Source/Runtime/Engine/Animation/BoneReference.h"

#include "RootMotionGeneratorOp.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class ERootMotionSource : uint8 {
    CopyFromSourceRoot,
    GenerateFromTargetPelvis,
};

UENUM()
enum class ERootMotionHeightSource : uint8 {
    CopyHeightFromSource,
    SnapToGround,
};

USTRUCT()
struct FIKRetargetRootMotionOpSettings : public FIKRetargetOpSettingsBase {
    GENERATED_BODY()
    UPROPERTY()
    FBoneReference SourceRoot;
    UPROPERTY()
    FBoneReference TargetRoot;
    UPROPERTY()
    ERootMotionSource RootMotionSource = ERootMotionSource::CopyFromSourceRoot;
    UPROPERTY()
    FBoneReference TargetPelvis;
    UPROPERTY()
    bool bRotateWithPelvis = false;
    UPROPERTY()
    ERootMotionHeightSource RootHeightSource = ERootMotionHeightSource::CopyHeightFromSource;
    UPROPERTY()
    FTransform GlobalOffset;
    UPROPERTY()
    bool bMaintainOffsetFromPelvis = true;
    UPROPERTY()
    bool bPropagateToNonRetargetedChildren = true;
    UPROPERTY()
    FName SourceRootBone_DEPRECATED;
    UPROPERTY()
    FName TargetRootBone_DEPRECATED;
    UPROPERTY()
    FName TargetPelvisBone_DEPRECATED;
};

USTRUCT()
struct FIKRetargetRootMotionOp : public FIKRetargetOpBase {
    GENERATED_BODY()
    UPROPERTY()
    FIKRetargetRootMotionOpSettings Settings;
};

}
